/*
 * Copyright (C) 2020-2021 Dmitriy Purgin <dpurgin@gmail.com>
 * Copyright (C) 2019-2021 Dmitriy Purgin <dmitriy.purgin@sequality.at>
 * Copyright (C) 2019-2021 sequality software engineering e.U. <office@sequality.at>
 *
 * This file is part of QtOrm library.
 *
 * QtOrm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QtOrm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with QtOrm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "qormpsqlprovider.h"

#include "qormentityinstancecache.h"
#include "qormquery.h"
#include "qormrelation.h"
#include "qormpsqlconfiguration.h"

#include "qormglobal_p.h"
#include "qormpsqlstatementgenerator_p.h"

#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlerror.h>
#include <QtSql/qsqlquery.h>
#include <QtSql/qsqlrecord.h>

QT_BEGIN_NAMESPACE

class QOrmPSQLProviderPrivate
{
    Q_DECLARE_PUBLIC(QOrmPSQLProvider)

    explicit QOrmPSQLProviderPrivate(const QOrmPSQLConfiguration& configuration,
                                           QOrmPSQLProvider* parent) :
        q_ptr{parent}, m_sqlConfiguration{configuration}
    {
    }

    QOrmPSQLProvider* q_ptr{nullptr};
    QSqlDatabase m_database;
    QOrmPSQLConfiguration m_sqlConfiguration;
    QSet<QString> m_schemaSyncCache;
    int m_transactionCounter{0};

    Q_REQUIRED_RESULT
    QOrmError lastDatabaseError() const;

    Q_REQUIRED_RESULT
    QSqlQuery prepareAndExecute(const QString& statement, const QVariantMap& parameters);

    QOrmError ensureSchemaSynchronized(const QOrmRelation& entityMetadata);
    QOrmError recreateSchema(const QOrmRelation& entityMetadata);
    QOrmError updateSchema(const QOrmRelation& entityMetadata);
    QOrmError validateSchema(const QOrmRelation& entityMetadata);
    QOrmError appendSchema(const QOrmRelation& entityMetadata);

    QOrmQueryResult<QObject> read(const QOrmQuery& query,
                                  QOrmEntityInstanceCache& entityInstanceCache);
    QOrmQueryResult<QObject> merge(const QOrmQuery& query);
    QOrmQueryResult<QObject> remove(const QOrmQuery& query);
};

QOrmError QOrmPSQLProviderPrivate::lastDatabaseError() const
{
    return QOrmError{QOrm::ErrorType::Provider, m_database.lastError().text()};
}

QSqlQuery QOrmPSQLProviderPrivate::prepareAndExecute(const QString& statement,
                                                     const QVariantMap& parameters = {})
{
    QSqlQuery query{m_database};

    if (m_sqlConfiguration.verbose())
        qCDebug(qtorm).noquote() << "Executing:" << statement;

    if (!query.prepare(statement))
        return query;

    if (!parameters.isEmpty())
    {
        if (m_sqlConfiguration.verbose())
            qCDebug(qtorm) << "Bound parameters:" << parameters;

        for (auto it = parameters.begin(); it != parameters.end(); ++it)
            query.bindValue(it.key(), it.value());
    }

    query.exec();

    return query;
}

QOrmError QOrmPSQLProviderPrivate::ensureSchemaSynchronized(const QOrmRelation& relation)
{
    switch (relation.type())
    {
        case QOrm::RelationType::Mapping:
        {
            Q_ASSERT(relation.mapping() != nullptr);

            if (m_schemaSyncCache.contains(relation.mapping()->className()))
                return {QOrm::ErrorType::None, ""};

            QOrmError error{QOrm::ErrorType::None, {}};

            QOrmPSQLConfiguration::SchemaMode effectiveSchemaMode =
                m_sqlConfiguration.schemaMode();

            if (relation.mapping()->userMetadata().contains(QOrm::Keyword::Schema))
            {
                static QMap<QString, QOrmPSQLConfiguration::SchemaMode> schemaModes{
                    {"recreate", QOrmPSQLConfiguration::SchemaMode::Recreate},
                    {"update", QOrmPSQLConfiguration::SchemaMode::Update},
                    {"validate", QOrmPSQLConfiguration::SchemaMode::Validate},
                    {"bypass", QOrmPSQLConfiguration::SchemaMode::Bypass},
                    {"append", QOrmPSQLConfiguration::SchemaMode::Append}};

                QString schemaModeValue =
                    relation.mapping()->userMetadata().value(QOrm::Keyword::Schema).toString();

                if (!schemaModes.contains(schemaModeValue))
                {
                    qFatal("QtOrm: Unsupported schema mode in %s: Q_ORM_CLASS(SCHEMA %s)",
                           qPrintable(relation.mapping()->className()),
                           qPrintable(schemaModeValue));
                }
                else
                {
                    effectiveSchemaMode = schemaModes.value(schemaModeValue);
                }
            }

            switch (effectiveSchemaMode)
            {
                case QOrmPSQLConfiguration::SchemaMode::Recreate:
                    error = recreateSchema(relation);
                    break;

                case QOrmPSQLConfiguration::SchemaMode::Update:
                    error = updateSchema(relation);
                    break;

                case QOrmPSQLConfiguration::SchemaMode::Validate:
                    error = validateSchema(relation);
                    break;

                case QOrmPSQLConfiguration::SchemaMode::Bypass:
                    // no error
                    break;

                case QOrmPSQLConfiguration::SchemaMode::Append:
                    error = appendSchema(relation);
                    break;
            }

            if (error.type() == QOrm::ErrorType::None)
            {
                m_schemaSyncCache.insert(relation.mapping()->className());

                for (const QOrmPropertyMapping& propertyMapping :
                     relation.mapping()->propertyMappings())
                {
                    if (propertyMapping.isReference())
                    {
                        QOrmError referencedError = ensureSchemaSynchronized(
                            QOrmRelation{*propertyMapping.referencedEntity()});

                        if (referencedError.type() != QOrm::ErrorType::None)
                            return referencedError;
                    }
                }
            }

            return error;
        }

        case QOrm::RelationType::Query:
            Q_ASSERT(relation.query() != nullptr);
            return ensureSchemaSynchronized(relation.query()->relation());
    }

    Q_ORM_UNEXPECTED_STATE;
}

QOrmError QOrmPSQLProviderPrivate::recreateSchema(const QOrmRelation& relation)
{
    Q_ASSERT(m_database.isOpen());
    Q_ASSERT(relation.type() == QOrm::RelationType::Mapping);
    Q_ASSERT(relation.mapping() != nullptr);

    if (m_database.tables().contains(relation.mapping()->tableName()))
    {
        QVariantMap boundParameters;
        QString statement =
            QOrmPSQLStatementGenerator::generateDropTableStatement(*relation.mapping(),
                                                                   boundParameters);

        QSqlQuery query = prepareAndExecute(statement, boundParameters);

        if (query.lastError().type() != QSqlError::NoError)
            return QOrmError{QOrm::ErrorType::UnsynchronizedSchema, query.lastError().text()};
    }

    QVariantMap boundParameters;
    QString statement =
        QOrmPSQLStatementGenerator::generateCreateTableStatement(*relation.mapping(),
                                                                 boundParameters);

    QSqlQuery query = prepareAndExecute(statement);

    if (query.lastError().type() != QSqlError::NoError)
        return QOrmError{QOrm::ErrorType::UnsynchronizedSchema, query.lastError().text()};

    return QOrmError{QOrm::ErrorType::None, ""};
}

QOrmQueryResult<QObject> QOrmPSQLProviderPrivate::read(
    const QOrmQuery& query,
    QOrmEntityInstanceCache& entityInstanceCache)
{
    Q_ASSERT(query.projection().has_value());

    auto [statement, boundParameters] = QOrmPSQLStatementGenerator::generate(query);

    QSqlQuery sqlQuery = prepareAndExecute(statement, boundParameters);

    if (sqlQuery.lastError().type() != QSqlError::NoError)
        return QOrmQueryResult<QObject>{
            QOrmError{QOrm::ErrorType::Provider, sqlQuery.lastError().text()}};

    QVector<QObject*> resultSet;

    const QOrmPropertyMapping* objectIdMapping = query.projection()->objectIdMapping();

    // If there is an object ID, compare the cached entities with the ones read from the
    // backend. If there is an inconsistency, it will be reported.
    // All read entities are replaced with their cached versions if found.
    if (objectIdMapping != nullptr)
    {
        while (sqlQuery.next())
        {
            QVariant objectId = sqlQuery.value(objectIdMapping->tableFieldName());

            QObject* cachedInstance = entityInstanceCache.get(*query.projection(), objectId);

            // cached instance: check if consistent
            if (cachedInstance != nullptr)
            {
                // If inconsistent, return an error. Already cached instances remain in the cache
                if (entityInstanceCache.isModified(cachedInstance) &&
                    !query.flags().testFlag(QOrm::QueryFlags::OverwriteCachedInstances))
                {
                    QString errorString;
                    QDebug dbg{&errorString};
                    dbg << "Entity instance" << cachedInstance
                        << "was read from the database but has unsaved changes in the "
                           "OR-mapper. "
                           "Merge this instance or discard changes before reading.";

                    return QOrmQueryResult<QObject>{
                        QOrmError{QOrm::ErrorType::UnsynchronizedEntity, errorString}};
                }
                else if (query.flags().testFlag(QOrm::QueryFlags::OverwriteCachedInstances))
                {
                    QOrmError error = fillEntityInstance(*query.projection(),
                                                         cachedInstance,
                                                         sqlQuery.record(),
                                                         entityInstanceCache,
                                                         query.flags());

                    if (error != QOrm::ErrorType::None)
                    {
                        entityInstanceCache.markUnmodified(cachedInstance);
                        return QOrmQueryResult<QObject>{error};
                    }
                }

                resultSet.push_back(cachedInstance);
            }
            // new instance: it will be cached in makeEntityInstance
            else
            {
                QOrmPrivate::Expected<QObject*, QOrmError> entityInstance =
                    makeEntityInstance(*query.projection(), sqlQuery.record(), entityInstanceCache);

                if (entityInstance)
                {
                    resultSet.push_back(entityInstance.value());
                }
                else
                {
                    return QOrmQueryResult<QObject>{entityInstance.error()};
                }
            }
        }
    }
    // No object ID in this projection: cannot cache, just return the results
    else
    {
        while (sqlQuery.next())
        {
            QOrmPrivate::Expected<QObject*, QOrmError> entityInstance =
                makeEntityInstance(*query.projection(), sqlQuery.record(), entityInstanceCache);

            if (entityInstance)
            {
                resultSet.push_back(entityInstance.value());
            }
            else
            {
                // if error occurred, delete everything that was read from the database since no
                // caching was involved
                qDeleteAll(resultSet);
                return QOrmQueryResult<QObject>{entityInstance.error()};
            }
        }
    }

    return QOrmQueryResult<QObject>{resultSet};
}

QOrmQueryResult<QObject> QOrmPSQLProviderPrivate::merge(const QOrmQuery& query)
{
    Q_ASSERT(query.relation().type() == QOrm::RelationType::Mapping);
    Q_ASSERT(query.entityInstance() != nullptr);

    auto [statement, boundParameters] = QOrmPSQLStatementGenerator::generate(query);

    QSqlQuery sqlQuery = prepareAndExecute(statement, boundParameters);

    if (sqlQuery.lastError().type() != QSqlError::NoError)
        return QOrmQueryResult<QObject>{{QOrm::ErrorType::Provider, sqlQuery.lastError().text()}};

    if (sqlQuery.numRowsAffected() != 1)
    {
        return QOrmQueryResult<QObject>{
            {QOrm::ErrorType::UnsynchronizedEntity, "Unexpected number of rows affected"}};
    }

    return QOrmQueryResult<QObject>{sqlQuery.lastInsertId()};
}

QOrmQueryResult<QObject> QOrmPSQLProviderPrivate::remove(const QOrmQuery& query)
{
    auto [statement, boundParameters] = QOrmPSQLStatementGenerator::generate(query);

    QSqlQuery sqlQuery = prepareAndExecute(statement, boundParameters);

    if (sqlQuery.lastError().type() != QSqlError::NoError)
        return QOrmQueryResult<QObject>{{QOrm::ErrorType::Provider, sqlQuery.lastError().text()}};

    return QOrmQueryResult<QObject>{sqlQuery.numRowsAffected()};
}

QOrmError QOrmPSQLProviderPrivate::setForeignKeysEnabled(bool enabled, QString tableName)
{
    QSqlQuery query = enabled ? m_database.exec("ALTER TABLE :table DISABLE TRIGGER ALL")
                              : m_database.exec("ALTER TABLE :table ENABLE TRIGGER ALL");
    query.addBindValue(tableName);

    if (query.lastError().type() != QSqlError::NoError)
    {
        return {QOrm::ErrorType::Provider, query.lastError().text()};
    }

    return {QOrm::ErrorType::None, {}};
}


QOrmPSQLProvider::QOrmPSQLProvider(const QOrmPSQLConfiguration &sqlConfiguration)
    : QOrmAbstractProvider{}
    , d_ptr{new QOrmPSQLProviderPrivate{sqlConfiguration, this}}

{
}

QOrmPSQLProvider::~QOrmPSQLProvider()
{
    delete d_ptr;
}

QOrmError QOrmPSQLProvider::connectToBackend()
{
    Q_D(QOrmPSQLProvider);

    if (!d->m_database.isOpen())
    {
        d->m_database = QSqlDatabase::addDatabase("QPSQL");
        d->m_database.setConnectOptions(d->m_sqlConfiguration.connectOptions());
        d->m_database.setDatabaseName(d->m_sqlConfiguration.databaseName());

        if (!d->m_database.open())
            return d->lastDatabaseError();
    }

    return QOrmError{QOrm::ErrorType::None, {}};
}

QOrmError QOrmPSQLProvider::disconnectFromBackend()
{
    Q_D(QOrmPSQLProvider);

    d->m_database.close();
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);

    return QOrmError{QOrm::ErrorType::None, {}};
}

bool QOrmPSQLProvider::isConnectedToBackend()
{
    Q_D(QOrmPSQLProvider);

    return d->m_database.isOpen();
}

QOrmError QOrmPSQLProvider::beginTransaction()
{
    Q_D(QOrmPSQLProvider);

    if (++d->m_transactionCounter == 1)
    {
        if (!d->m_database.transaction())
        {
            QSqlError error = d->m_database.lastError();

            if (error.type() != QSqlError::NoError)
                return d->lastDatabaseError();
            else
                return QOrmError{QOrm::ErrorType::Other,
                                 QStringLiteral("Unable to start transaction")};
        }
    }

    return QOrmError{QOrm::ErrorType::None, {}};
}

QOrmError QOrmPSQLProvider::commitTransaction()
{
    Q_D(QOrmPSQLProvider);

    if (--d->m_transactionCounter == 0)
    {
        if (!d->m_database.commit())
        {
            QSqlError error = d->m_database.lastError();

            if (error.type() != QSqlError::NoError)
                return d->lastDatabaseError();
            else
                return QOrmError{QOrm::ErrorType::Other,
                                 QStringLiteral("Unable to commit transaction")};
        }
    }

    return QOrmError{QOrm::ErrorType::None, {}};
}

QOrmError QOrmPSQLProvider::rollbackTransaction()
{
    Q_D(QOrmPSQLProvider);

    if (--d->m_transactionCounter == 0)
    {
        if (!d->m_database.rollback())
        {
            QSqlError error = d->m_database.lastError();

            if (error.type() != QSqlError::NoError)
                return d->lastDatabaseError();
            else
                return QOrmError{QOrm::ErrorType::Other,
                                 QStringLiteral("Unable to rollback transaction")};
        }
    }

    return QOrmError{QOrm::ErrorType::None, {}};
}

QOrmQueryResult<QObject> QOrmPSQLProvider::execute(const QOrmQuery &query, QOrmEntityInstanceCache &entityInstanceCache)
{
    Q_D(QOrmPSQLProvider);

    QOrmError error = d->ensureSchemaSynchronized(query.relation());

    if (error.type() != QOrm::ErrorType::None)
    {
        return QOrmQueryResult<QObject>{error};
    }

    switch (query.operation())
    {
        case QOrm::Operation::Read:
            return d->read(query, entityInstanceCache);

        case QOrm::Operation::Create:
        case QOrm::Operation::Update:
            return d->merge(query);

        case QOrm::Operation::Delete:
            return d->remove(query);

        case QOrm::Operation::Merge:
            Q_ORM_UNEXPECTED_STATE;
    }

    Q_ORM_UNEXPECTED_STATE;
}

QOrmPSQLConfiguration QOrmPSQLProvider::configuration() const
{
    Q_D(const QOrmPSQLProvider);

    return d->m_sqlConfiguration;
}

QSqlDatabase QOrmPSQLProvider::database() const
{
    Q_D(const QOrmPSQLProvider);

    return d->m_database;
}

QT_END_NAMESPACE

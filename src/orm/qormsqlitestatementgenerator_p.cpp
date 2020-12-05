/*
 * Copyright (C) 2019 Dmitriy Purgin <dmitriy.purgin@sequality.at>
 * Copyright (C) 2019 sequality software engineering e.U. <office@sequality.at>
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

#include "qormsqlitestatementgenerator_p.h"
#include "qormfilter.h"
#include "qormfilterexpression.h"
#include "qormglobal_p.h"
#include "qormmetadata.h"
#include "qormmetadatacache.h"
#include "qormorder.h"
#include "qormquery.h"
#include "qormrelation.h"

#include <QtCore/qstringbuilder.h>

QT_BEGIN_NAMESPACE

static QString insertParameter(QVariantMap& boundParameters, QString name, QVariant value)
{
    QString key = ':' % name;

    for (int i = 0; boundParameters.contains(key); ++i)
        key = ':' % name % QString::number(i);

    boundParameters.insert(key, value);

    return key;    
}

static QVariant propertyValueForQuery(const QObject* entityInstance,
                                      const QOrmPropertyMapping& propertyMapping)
{
    if (propertyMapping.isReference() && !propertyMapping.isTransient())
    {
        const QOrmMetadata* referencedEntity = propertyMapping.referencedEntity();

        Q_ASSERT(referencedEntity != nullptr);
        Q_ASSERT(referencedEntity->objectIdMapping() != nullptr);

        const QObject* referencedInstance =
            QOrmPrivate::propertyValue(entityInstance, propertyMapping.classPropertyName())
                .value<QObject*>();

        return referencedInstance == nullptr
                   ? QVariant::fromValue(nullptr)
                   : QOrmPrivate::objectIdPropertyValue(referencedInstance, *referencedEntity);
    }
    else
    {
        return QOrmPrivate::propertyValue(entityInstance, propertyMapping.classPropertyName());
    }
}

std::pair<QString, QVariantMap> QOrmSqliteStatementGenerator::generate(const QOrmQuery& query)
{
    QVariantMap boundParameters;
    QString statement = generate(query, boundParameters);

    return std::make_pair(statement, boundParameters);
}

QString QOrmSqliteStatementGenerator::generate(const QOrmQuery& query, QVariantMap& boundParameters)
{
    switch (query.operation())
    {
        case QOrm::Operation::Create:
            return generateInsertStatement(*query.relation().mapping(),
                                           query.entityInstance(),
                                           boundParameters);

        case QOrm::Operation::Update:
            return generateUpdateStatement(*query.relation().mapping(),
                                           query.entityInstance(),
                                           boundParameters);

        case QOrm::Operation::Read:
            return generateSelectStatement(query, boundParameters);

        case QOrm::Operation::Delete:
            Q_ASSERT(query.relation().type() == QOrm::RelationType::Mapping);

            if (query.entityInstance() != nullptr)
            {
                return generateDeleteStatement(*query.relation().mapping(),
                                               query.entityInstance(),
                                               boundParameters);
            }
            else if (query.filter().has_value())
            {
                return generateDeleteStatement(*query.relation().mapping(),
                                               *query.filter(),
                                               boundParameters);
            }
            else
                Q_ORM_UNEXPECTED_STATE;

        default:
            Q_ORM_UNEXPECTED_STATE;
    }
}

QString QOrmSqliteStatementGenerator::generateInsertStatement(const QOrmMetadata& relation,
                                                              const QObject* entityInstance,
                                                              QVariantMap& boundParameters)
{
    QStringList fieldsList;
    QStringList valuesList;

    for (const QOrmPropertyMapping& propertyMapping : relation.propertyMappings())
    {
        if (propertyMapping.isAutogenerated() || propertyMapping.isTransient())
            continue;

        QVariant propertyValue = propertyValueForQuery(entityInstance, propertyMapping);

        QString valueStr =
            insertParameter(boundParameters, propertyMapping.tableFieldName(), propertyValue);

        fieldsList.push_back(propertyMapping.tableFieldName());
        valuesList.push_back(valueStr);
    }

    QString fieldsStr = fieldsList.join(',');
    QString valuesStr = valuesList.join(',');

    QString statement = QStringLiteral("INSERT INTO %1(%2) VALUES(%3)")
                            .arg(relation.tableName(), fieldsStr, valuesStr);

    return statement;
}

QString QOrmSqliteStatementGenerator::generateUpdateStatement(const QOrmMetadata& relation,
                                                              const QObject* entityInstance,
                                                              QVariantMap& boundParameters)
{
    if (relation.objectIdMapping() == nullptr)
        qFatal("QtORM: Unable to update entity without object ID property");

    QStringList setList;

    for (const QOrmPropertyMapping& propertyMapping : relation.propertyMappings())
    {
        if (propertyMapping.isTransient() || propertyMapping.isObjectId())
            continue;

        QVariant propertyValue = propertyValueForQuery(entityInstance, propertyMapping);

        QString parameterName =
            insertParameter(boundParameters, propertyMapping.tableFieldName(), propertyValue);
        setList.push_back(QString{"%1 = %2"}.arg(propertyMapping.tableFieldName(), parameterName));
    }

    QVariant objectId = QOrmPrivate::objectIdPropertyValue(entityInstance, relation);

    QString whereClause =
        generateWhereClause(QOrmFilter{*relation.objectIdMapping() == objectId}, boundParameters);

    QStringList parts = {"UPDATE", relation.tableName(), "SET", setList.join(','), whereClause};

    return parts.join(QChar(' '));
}

QString QOrmSqliteStatementGenerator::generateSelectStatement(const QOrmQuery& query,
                                                              QVariantMap& boundParameters)
{
    Q_ASSERT(query.operation() == QOrm::Operation::Read);

    QStringList parts = {"SELECT *", generateFromClause(query.relation(), boundParameters)};

    if (query.filter().has_value())
        parts += generateWhereClause(*query.filter(), boundParameters);

    parts += generateOrderClause(query.order());

    return parts.join(QChar{' '});
}

QString QOrmSqliteStatementGenerator::generateDeleteStatement(const QOrmMetadata& relation,
                                                              const QOrmFilter& filter,
                                                              QVariantMap& boundParameters)
{
    QStringList parts = {"DELETE",
                         generateFromClause(QOrmRelation{relation}, boundParameters),
                         generateWhereClause(filter, boundParameters)};

    return parts.join(QChar{' '});
}

QString QOrmSqliteStatementGenerator::generateDeleteStatement(const QOrmMetadata& relation,
                                                              const QObject* instance,
                                                              QVariantMap& boundParameters)
{
    Q_ASSERT(relation.objectIdMapping() != nullptr);

    QVariant objectId = QOrmPrivate::objectIdPropertyValue(instance, relation);

    return generateDeleteStatement(relation,
                                   QOrmFilter{*relation.objectIdMapping() == objectId},
                                   boundParameters);
}

QString QOrmSqliteStatementGenerator::generateFromClause(const QOrmRelation& relation,
                                                         QVariantMap& boundParameters)
{
    switch (relation.type())
    {
        case QOrm::RelationType::Mapping:
            return QString{"FROM %1"}.arg(relation.mapping()->tableName());

        case QOrm::RelationType::Query:
            Q_ASSERT(relation.query()->operation() == QOrm::Operation::Read);

            return QString{"FROM (%1)"}.arg(generate(*relation.query(), boundParameters));
    }

    Q_ORM_UNEXPECTED_STATE;
}

QString QOrmSqliteStatementGenerator::generateWhereClause(const QOrmFilter& filter,
                                                          QVariantMap& boundParameters)
{
    QString whereClause;

    if (filter.type() == QOrm::FilterType::Expression)
    {
        Q_ASSERT(filter.expression() != nullptr);

        whereClause = generateCondition(*filter.expression(), boundParameters);

        if (!whereClause.isEmpty())
            whereClause = "WHERE " + whereClause;
    }

    return whereClause;
}

QString QOrmSqliteStatementGenerator::generateOrderClause(const std::vector<QOrmOrder>& order)
{
    QStringList parts;

    for (const QOrmOrder& element : order)
    {
        parts += element.mapping().tableFieldName() % (element.direction() == Qt::AscendingOrder
                                                           ? QStringLiteral(" ASC")
                                                           : QStringLiteral(" DESC"));
    }

    return parts.empty() ? QString{} : QStringLiteral("ORDER BY ") % parts.join(',');
}

QString QOrmSqliteStatementGenerator::generateCondition(const QOrmFilterExpression& expression,
                                                        QVariantMap& boundParameters)
{
    switch (expression.type())
    {
        case QOrm::FilterExpressionType::TerminalPredicate:
            Q_ASSERT(expression.terminalPredicate() != nullptr);
            return generateCondition(*expression.terminalPredicate(), boundParameters);

        case QOrm::FilterExpressionType::BinaryPredicate:
            Q_ASSERT(expression.binaryPredicate() != nullptr);
            return generateCondition(*expression.binaryPredicate(), boundParameters);

        case QOrm::FilterExpressionType::UnaryPredicate:
            Q_ASSERT(expression.unaryPredicate() != nullptr);
            return generateCondition(*expression.unaryPredicate(), boundParameters);
    }

    Q_ORM_UNEXPECTED_STATE;
}

QString
QOrmSqliteStatementGenerator::generateCondition(const QOrmFilterTerminalPredicate& predicate,
                                                QVariantMap& boundParameters)
{
    Q_ASSERT(predicate.isResolved());

    static const QHash<QOrm::Comparison, QString> comparisonOps = {
        {QOrm::Comparison::Less, "<"},
        {QOrm::Comparison::Equal, "="},
        {QOrm::Comparison::Greater, ">"},
        {QOrm::Comparison::NotEqual, "<>"},
        {QOrm::Comparison::LessOrEqual, "<="},
        {QOrm::Comparison::GreaterOrEqual, ">="}};

    Q_ASSERT(comparisonOps.contains(predicate.comparison()));

    QVariant value;

    if (predicate.propertyMapping()->isReference())
    {
        const QOrmMetadata* referencedEntity = predicate.propertyMapping()->referencedEntity();
        Q_ASSERT(referencedEntity != nullptr);

        auto referencedInstance = predicate.value().value<QObject*>();
        Q_ASSERT(referencedInstance != nullptr);

        value = QOrmPrivate::objectIdPropertyValue(referencedInstance, *referencedEntity);
    }
    else
    {
        value = predicate.value();
    }

    QString parameterKey =
        insertParameter(boundParameters, predicate.propertyMapping()->tableFieldName(), value);

    QString statement = QString{"%1 %2 %3"}.arg(predicate.propertyMapping()->tableFieldName(),
                                                comparisonOps[predicate.comparison()],
                                                parameterKey);

    return statement;
}

QString QOrmSqliteStatementGenerator::generateCondition(const QOrmFilterBinaryPredicate& predicate,
                                                        QVariantMap& boundParameters)
{
    QString lhsExpr = generateCondition(predicate.lhs(), boundParameters);
    QString rhsExpr = generateCondition(predicate.rhs(), boundParameters);

    QString op;

    switch (predicate.logicalOperator())
    {
        case QOrm::BinaryLogicalOperator::Or:
            op = "OR";
            break;
        case QOrm::BinaryLogicalOperator::And:
            op = "AND";
            break;
    }

    Q_ASSERT(!op.isEmpty());

    return QString{"(%1) %2 (%3)"}.arg(lhsExpr, op, rhsExpr);
}

QString QOrmSqliteStatementGenerator::generateCondition(const QOrmFilterUnaryPredicate& predicate,
                                                        QVariantMap& boundParameters)
{
    QString rhsExpr = generateCondition(predicate.rhs(), boundParameters);
    Q_ASSERT(predicate.logicalOperator() == QOrm::UnaryLogicalOperator::Not);

    return QString{"NOT (%1)"}.arg(rhsExpr);
}

QString QOrmSqliteStatementGenerator::generateCreateTableStatement(const QOrmMetadata& entity)
{
    QStringList fields;

    for (const QOrmPropertyMapping& mapping : entity.propertyMappings())
    {
        if (mapping.isTransient())
            continue;

        QStringList columnDefs;

        if (mapping.isReference())
        {
            Q_ASSERT(mapping.referencedEntity()->objectIdMapping() != nullptr);

            columnDefs += {mapping.tableFieldName(),
                           toSqliteType(mapping.referencedEntity()->objectIdMapping()->dataType())};
        }
        else
        {
            columnDefs += {mapping.tableFieldName(), toSqliteType(mapping.dataType())};

            if (mapping.isObjectId())
                columnDefs.push_back(QStringLiteral("PRIMARY KEY"));

            if (mapping.isAutogenerated())
                columnDefs.push_back(QStringLiteral("AUTOINCREMENT"));
        }

        fields.push_back(columnDefs.join(' '));
    }

    QString fieldsStr = fields.join(',');

    return QStringLiteral("CREATE TABLE %1(%2)").arg(entity.tableName(), fieldsStr);
}

QString QOrmSqliteStatementGenerator::generateDropTableStatement(const QOrmMetadata& entity)
{
    return QStringLiteral("DROP TABLE %1").arg(entity.tableName());
}

QString QOrmSqliteStatementGenerator::toSqliteType(QVariant::Type type)
{
    // SQLite data types: https://sqlite.org/datatype3.html
    switch (type)
    {
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
            return QStringLiteral("INTEGER");

        case QVariant::Double:
            return QStringLiteral("REAL");

        case QVariant::Bool:
        case QVariant::Date:
        case QVariant::Time:
        case QVariant::DateTime:
            return QStringLiteral("NUMERIC");

        case QVariant::Char:
        case QVariant::String:
            return QStringLiteral("TEXT");

        default:
            // Additional check for type long.
            // There is no QVariant::Long but the type returned for long properties is
            // QMetaType::Long which is 32.
            if (static_cast<int>(type) == 32)
            {
                return QStringLiteral("INTEGER");
            }

            return QStringLiteral("BLOB");
    }
}

QT_END_NAMESPACE

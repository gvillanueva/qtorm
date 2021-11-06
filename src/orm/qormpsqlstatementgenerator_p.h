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

#ifndef QORMPSQLSTATEMENTGENERATOR_P_H
#define QORMPSQLSTATEMENTGENERATOR_P_H

#include <QtOrm/qormglobal.h>

#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QOrmMetadata;
class QOrmQuery;

class Q_ORM_EXPORT QOrmPSQLStatementGenerator
{
public:
    [[nodiscard]] static std::pair<QString, QVariantMap> generate(const QOrmQuery& query);

    [[nodiscard]] static QString generate(const QOrmQuery& query, QVariantMap& boundParameters);

    [[nodiscard]] static QString generateInsertStatement(const QOrmMetadata& relation,
                                                         const QObject* instance,
                                                         QVariantMap& boundParameters);

    [[nodiscard]] static QString toPostgreSqlType(QVariant::Type type);

    [[nodiscard]] static QString generateCreateTableStatement(
        const QOrmMetadata& entity,
        std::optional<QString> overrideTableName = std::nullopt);

    [[nodiscard]] static QString generateDropTableStatement(const QOrmMetadata& entity,
                                                            QVariantMap& boundParameters);
};

QT_END_NAMESPACE

#endif // QORMPSQLSTATEMENTGENERATOR_P_H

/*
 * Copyright (C) 2020-2021 Dmitriy Purgin <dpurgin@gmail.com>
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

#ifndef QORMPSQLPROVIDER_H
#define QORMPSQLPROVIDER_H

#include <QtOrm/qormabstractprovider.h>
#include <QtOrm/qormglobal.h>
#include <QtOrm/qormqueryresult.h>

QT_BEGIN_NAMESPACE

class QOrmEntityInstanceCache;
class QOrmPSQLConfiguration;
class QOrmPSQLProviderPrivate;
class QSqlDatabase;

class Q_ORM_EXPORT QOrmPSQLProvider : public QOrmAbstractProvider
{
public:
    explicit QOrmPSQLProvider(const QOrmPSQLConfiguration &sqlConfiguration);
    ~QOrmPSQLProvider() override;

    QOrmError connectToBackend() override;
    QOrmError disconnectFromBackend() override;
    bool isConnectedToBackend() override;

    QOrmError beginTransaction() override;
    QOrmError commitTransaction() override;
    QOrmError rollbackTransaction() override;

    QOrmQueryResult<QObject> execute(const QOrmQuery &query,
                                     QOrmEntityInstanceCache &entityInstanceCache) override;

    QOrmPSQLConfiguration configuration() const;
    QSqlDatabase database() const;

private:
    Q_DECLARE_PRIVATE(QOrmPSQLProvider)
    QOrmPSQLProviderPrivate* d_ptr{nullptr};
};

QT_END_NAMESPACE

#endif // QORMPSQLPROVIDER_H
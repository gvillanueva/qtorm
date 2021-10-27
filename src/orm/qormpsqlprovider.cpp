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

#include "qormpsqlconfiguration.h"

#include "qormpsqlstatementgenerator_p.h"

#include <QtSql/qsqldatabase.h>

QT_BEGIN_NAMESPACE

class QOrmPSQLProviderPrivate
{
    Q_DECLARE_PUBLIC(QOrmPSQLProvider)

    explicit QOrmPSQLProviderPrivate(const QOrmPSQLConfiguration& configuration,
                                           QOrmPSQLProvider* parent) :
        q_ptr{parent}, m_configuration{configuration}
    {
    }

    QOrmPSQLProvider* q_ptr{nullptr};
    QSqlDatabase m_database;
    QOrmPSQLConfiguration m_configuration;
};

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
}

QOrmError QOrmPSQLProvider::disconnectFromBackend()
{
    Q_D(QOrmPSQLProvider);
}

bool QOrmPSQLProvider::isConnectedToBackend()
{
    Q_D(QOrmPSQLProvider);
}

QOrmError QOrmPSQLProvider::beginTransaction()
{
    Q_D(QOrmPSQLProvider);
}

QOrmError QOrmPSQLProvider::commitTransaction()
{
    Q_D(QOrmPSQLProvider);
}

QOrmError QOrmPSQLProvider::rollbackTransaction()
{
    Q_D(QOrmPSQLProvider);
}

QOrmQueryResult<QObject> QOrmPSQLProvider::execute(const QOrmQuery &query, QOrmEntityInstanceCache &entityInstanceCache)
{
    Q_D(QOrmPSQLProvider);
}

QOrmPSQLConfiguration QOrmPSQLProvider::configuration() const
{
    Q_D(const QOrmPSQLProvider);

    return d->m_configuration;
}

QSqlDatabase QOrmPSQLProvider::database() const
{
    Q_D(const QOrmPSQLProvider);

    return d->m_database;
}

QT_END_NAMESPACE

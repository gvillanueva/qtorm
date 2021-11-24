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

#include "qormpsqlconfiguration.h"

QT_BEGIN_NAMESPACE

QString QOrmPSQLConfiguration::hostName() const
{
    return m_hostName;
}

void QOrmPSQLConfiguration::setHostName(const QString &hostName)
{
    m_hostName = hostName;
}

QString QOrmPSQLConfiguration::connectOptions() const
{
    return m_connectOptions;
}

void QOrmPSQLConfiguration::setConnectOptions(const QString& connectOptions)
{
    m_connectOptions = connectOptions;
}

QString QOrmPSQLConfiguration::databaseName() const
{
    return m_databaseName;
}

void QOrmPSQLConfiguration::setDatabaseName(const QString& databaseName)
{
    m_databaseName = databaseName;
}

bool QOrmPSQLConfiguration::verbose() const
{
    return m_verbose;
}

void QOrmPSQLConfiguration::setVerbose(bool verbose)
{
    m_verbose = verbose;
}

QOrmPSQLConfiguration::SchemaMode QOrmPSQLConfiguration::schemaMode() const
{
    return m_schemaMode;
}

void QOrmPSQLConfiguration::setSchemaMode(SchemaMode schemaMode)
{
    m_schemaMode = schemaMode;
}

QString QOrmPSQLConfiguration::userName() const
{
    return m_userName;
}

void QOrmPSQLConfiguration::setUserName(const QString &userName)
{
    m_userName = userName;
}

QString QOrmPSQLConfiguration::password() const
{
    return m_password;
}

void QOrmPSQLConfiguration::setPassword(const QString &password)
{
    m_password = password;
}

QT_END_NAMESPACE

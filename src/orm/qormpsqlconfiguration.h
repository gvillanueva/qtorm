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

#ifndef QORMPSQLCONFIGURATION_H
#define QORMPSQLCONFIGURATION_H

#include <QtCore/qstring.h>
#include <QtOrm/qormglobal.h>

QT_BEGIN_NAMESPACE

class Q_ORM_EXPORT QOrmPSQLConfiguration
{
public:
    enum class SchemaMode
    {
        Recreate,
        Update,
        Validate,
        Bypass,
        Append
    };

public:
    Q_REQUIRED_RESULT
    QString hostName() const;
    void setHostName(const QString& hostName);

    Q_REQUIRED_RESULT
    QString connectOptions() const;
    void setConnectOptions(const QString& connectOptions);

    Q_REQUIRED_RESULT
    QString databaseName() const;
    void setDatabaseName(const QString& databaseName);

    Q_REQUIRED_RESULT
    bool verbose() const;
    void setVerbose(bool verbose);

    Q_REQUIRED_RESULT
    SchemaMode schemaMode() const;
    void setSchemaMode(SchemaMode schemaMode);

    Q_REQUIRED_RESULT
    QString userName() const;
    void setUserName(const QString& userName);

    Q_REQUIRED_RESULT
    QString password() const;
    void setPassword(const QString& password);

private:
    QString m_hostName;
    QString m_connectOptions;
    QString m_databaseName;
    bool m_verbose{false};
    SchemaMode m_schemaMode;
    QString m_userName;
    QString m_password;
};

QT_END_NAMESPACE

#endif // QORMPSQLCONFIGURATION_H
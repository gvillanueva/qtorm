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

#include "qormabstractprovider.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOrmAbstractProvider

    \inmodule QtOrm
    \brief The QOrmAbstractProvider provides the abstract interface for
    database-specific backends.

    The QOrmAbstractItemModel class defines teh standard interface that ORM
    backends must use to be able to interoperate with QOrmSession manager.
*/

/*!
    \fn QOrmAbstractProvider::~QOrmAbstractProvider

    Destroys the object and frees any allocated resources.
*/

/*!
    \fn QOrmAbstractProvider::connectToBackend()

    Creates a connection to the backend using current connection values.
*/

/*!
    \fn QOrmAbstractProvider::disconnectFromBackend()

    Disconnects from the backend, freeing any resources acquired.
*/

/*!
    \fn QOrmAbstractProvider::isConnectedToBackend()

    Returns true if the backend connection is currently open; otherwise returns
    false.
*/

/*!
    \fn QOrmAbstractProvider::beginTransaction()

    Begins a transaction on the backend if the provider supports transactions.
    Returns QOrm::ErrorType::None if the operation succeeded. Otherwise it
    returns a QOrmError initialized to the error creating transaction.
*/

/*!
    \fn QOrmAbstractProvider::commitTransaction()

    Commits a transaction to the backend if the provider supports transactions
    and a transaction has been started. Returns QOrm::ErrorType::None if the
    operation succeeded. Otherwise it returns a QOrmError initialized to the
    error committing transaction.
*/

/*!
    \fn QOrmAbstractProvider::rollbackTransaction()

    Rolls back a transaction on the backend if the provider supports
    transactions and a transaction has been started. Returns
    QOrm::ErrorType::None if the operation succeeded. Otherwise it returns a
    QOrmError initialized to the error rolling back transaction.
*/

/*!
    \fn QOrmAbstractProvider::execute(const QOrmQuery& query,
                                      QOrmEntityInstanceCache& entityInstanceCache)

    Executes an ORM query on the backend and returns a QOrmQueryResult object.
*/

QOrmAbstractProvider::~QOrmAbstractProvider() = default;

QT_END_NAMESPACE

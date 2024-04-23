/*
 * Copyright (C) 2019-2024 Dmitriy Purgin <dmitriy.purgin@sequality.at>
 * Copyright (C) 2019-2024 sequality software engineering e.U. <office@sequality.at>
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

#include <QtTest>

#include <QOrmMetadataCache>

#include "domain/person.h"
#include "domain/province.h"
#include "domain/town.h"
#include "domain/withenum.h"

class MetadataCacheTest : public QObject
{
    Q_OBJECT

public:
    MetadataCacheTest();
    ~MetadataCacheTest();

private slots:
    void initTestCase();

    void testDefaultMetadata();
    void testOneToOneReference();
    void testManyToOneReference();

    void testCustomizedEntity();
    void testColumnNameForColumnWithReference();
    void testNamespacedEntity();

    void testEnumColumn();
    void testColumnWithNamespacedReference();
};

MetadataCacheTest::MetadataCacheTest()
{
}

MetadataCacheTest::~MetadataCacheTest()
{
}

void MetadataCacheTest::initTestCase()
{
    qRegisterOrmEntity<Town, Person>();
}

void MetadataCacheTest::testDefaultMetadata()
{
    QOrmMetadataCache cache;
    const QOrmMetadata& metadata = cache.get<Province>();

    QCOMPARE(metadata.className(), "Province");
    QCOMPARE(metadata.tableName(), "Province");

    QVERIFY(metadata.objectIdMapping() != nullptr);
    QCOMPARE(metadata.objectIdMapping()->dataType(), QVariant::Int);
    QVERIFY(metadata.objectIdMapping()->isObjectId());
    QVERIFY(metadata.objectIdMapping()->isAutogenerated());
    QCOMPARE(metadata.objectIdMapping()->classPropertyName(), "id");
    QCOMPARE(metadata.objectIdMapping()->tableFieldName(), "id");

    auto mappings = metadata.propertyMappings();
    QCOMPARE(mappings.size(), 2u);

    QCOMPARE(mappings[0].tableFieldName(), "id");
    QCOMPARE(mappings[0].classPropertyName(), "id");
    QCOMPARE(mappings[0].dataType(), QVariant::Int);
    QVERIFY(mappings[0].isObjectId());
    QVERIFY(mappings[0].isAutogenerated());
    QVERIFY(!mappings[0].isReference());

    QCOMPARE(mappings[1].tableFieldName(), "name");
    QCOMPARE(mappings[1].classPropertyName(), "name");
    QCOMPARE(mappings[1].dataType(), QVariant::String);
    QVERIFY(!mappings[1].isObjectId());
    QVERIFY(!mappings[1].isAutogenerated());
    QVERIFY(!mappings[1].isReference());

    {
        auto mapping = metadata.classPropertyMapping("id");
        QVERIFY(mapping != nullptr);
        QCOMPARE(mapping->tableFieldName(), "id");
        QCOMPARE(mapping->classPropertyName(), "id");
        QCOMPARE(mapping->dataType(), QVariant::Int);
        QVERIFY(mapping->isObjectId());
        QVERIFY(mapping->isAutogenerated());
        QVERIFY(!mapping->isReference());
    }

    {
        auto mapping = metadata.classPropertyMapping("name");
        QVERIFY(mapping != nullptr);
        QCOMPARE(mapping->tableFieldName(), "name");
        QCOMPARE(mapping->classPropertyName(), "name");
        QCOMPARE(mapping->dataType(), QVariant::String);
        QVERIFY(!mapping->isObjectId());
        QVERIFY(!mapping->isAutogenerated());
        QVERIFY(!mapping->isReference());
    }

    {
        auto mapping = metadata.tableFieldMapping("id");
        QVERIFY(mapping != nullptr);
        QCOMPARE(mapping->tableFieldName(), "id");
        QCOMPARE(mapping->classPropertyName(), "id");
        QCOMPARE(mapping->dataType(), QVariant::Int);
        QVERIFY(mapping->isObjectId());
        QVERIFY(mapping->isAutogenerated());
        QVERIFY(!mapping->isReference());
    }

    {
        auto mapping = metadata.tableFieldMapping("name");
        QVERIFY(mapping != nullptr);
        QCOMPARE(mapping->tableFieldName(), "name");
        QCOMPARE(mapping->classPropertyName(), "name");
        QCOMPARE(mapping->dataType(), QVariant::String);
        QVERIFY(!mapping->isObjectId());
        QVERIFY(!mapping->isAutogenerated());
        QVERIFY(!mapping->isReference());
    }
}

void MetadataCacheTest::testOneToOneReference()
{
    QOrmMetadataCache cache;
    QOrmMetadata personMetadata = cache.get<Person>();

    QCOMPARE(personMetadata.className(), "Person");
    QCOMPARE(personMetadata.tableName(), "Person");

    auto provincePropertyMapping = personMetadata.classPropertyMapping("town");

    QCOMPARE(provincePropertyMapping->classPropertyName(), "town");
    QCOMPARE(provincePropertyMapping->tableFieldName(), "town_id");
    QCOMPARE(provincePropertyMapping->dataType(), QVariant::UserType);
    QVERIFY(provincePropertyMapping->isReference());
    QVERIFY(provincePropertyMapping->referencedEntity() != nullptr);
    QCOMPARE(provincePropertyMapping->referencedEntity()->className(), "Town");
}

void MetadataCacheTest::testManyToOneReference()
{
    QOrmMetadataCache cache;
    QOrmMetadata townMetadata = cache.get<Town>();

    QCOMPARE(townMetadata.className(), "Town");
    QCOMPARE(townMetadata.tableName(), "Town");

    auto populationPropertyMapping = townMetadata.classPropertyMapping("population");

    QCOMPARE(populationPropertyMapping->classPropertyName(), "population");
    QVERIFY(populationPropertyMapping->tableFieldName().isEmpty());
    QCOMPARE(populationPropertyMapping->dataType(), QVariant::UserType);
    QVERIFY(populationPropertyMapping->isReference());
    QVERIFY(populationPropertyMapping->isTransient());
    QVERIFY(populationPropertyMapping->referencedEntity() != nullptr);
    QCOMPARE(populationPropertyMapping->referencedEntity()->className(), "Person");
}

class CustomizedEntity : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool transientProperty READ transientProperty NOTIFY transientPropertyChanged)
    Q_PROPERTY(int anotherTransientProperty READ anotherTransientProperty NOTIFY
                   anotherTransientPropertyChanged STORED false)

    Q_ORM_CLASS(TABLE entity SCHEMA update)
    Q_ORM_PROPERTY(index COLUMN entity_id IDENTITY AUTOGENERATED false)
    Q_ORM_PROPERTY(transientProperty TRANSIENT)

public:
    Q_INVOKABLE CustomizedEntity() = default;

    int index() const { return m_index; }
    void setIndex(int index)
    {
        if (m_index != index)
        {
            m_index = index;
            emit indexChanged();
        }
    }

    QString name() const { return m_name; }
    void setName(QString name)
    {
        if (m_name != name)
        {
            m_name = name;
            emit nameChanged();
        }
    }

    bool transientProperty() const { return true; }
    bool anotherTransientProperty() const { return false; }

signals:
    void indexChanged();
    void nameChanged();
    void transientPropertyChanged();
    void anotherTransientPropertyChanged();

private:
    int m_index;
    QString m_name;
};

void MetadataCacheTest::testCustomizedEntity()
{
    qRegisterOrmEntity<CustomizedEntity>();

    QOrmMetadataCache cache;
    QOrmMetadata meta = cache.get<CustomizedEntity>();

    QCOMPARE(meta.className(), "CustomizedEntity");
    QCOMPARE(meta.tableName(), "entity");

    QCOMPARE(meta.userMetadata().size(), 2);
    QCOMPARE(meta.userMetadata().value(QOrm::Keyword::Table, ""), "entity");
    QCOMPARE(meta.userMetadata().value(QOrm::Keyword::Schema, ""), "update");

    auto indexPropertyMapping = meta.classPropertyMapping("index");
    QVERIFY(indexPropertyMapping != nullptr);
    QCOMPARE(indexPropertyMapping->classPropertyName(), "index");
    QCOMPARE(indexPropertyMapping->tableFieldName(), "entity_id");
    QCOMPARE(indexPropertyMapping->isObjectId(), true);
    QCOMPARE(indexPropertyMapping->isAutogenerated(), false);
    QCOMPARE(indexPropertyMapping->isTransient(), false);

    QCOMPARE(indexPropertyMapping->userMetadata().size(), 4);
    QCOMPARE(indexPropertyMapping->userMetadata().value(QOrm::Keyword::Property, ""), "index");
    QCOMPARE(indexPropertyMapping->userMetadata().value(QOrm::Keyword::Column, ""), "entity_id");
    QCOMPARE(indexPropertyMapping->userMetadata().value(QOrm::Keyword::Identity, false), true);
    QCOMPARE(indexPropertyMapping->userMetadata().value(QOrm::Keyword::Autogenerated, true), false);

    auto transientPropertyMapping = meta.classPropertyMapping("transientProperty");
    QVERIFY(transientPropertyMapping != nullptr);
    QCOMPARE(transientPropertyMapping->classPropertyName(), "transientProperty");
    QCOMPARE(transientPropertyMapping->tableFieldName(), "transientproperty");
    QCOMPARE(transientPropertyMapping->isObjectId(), false);
    QCOMPARE(transientPropertyMapping->isAutogenerated(), false);
    QCOMPARE(transientPropertyMapping->isTransient(), true);

    auto anotherTransientPropertyMapping = meta.classPropertyMapping("anotherTransientProperty");
    QVERIFY(anotherTransientPropertyMapping != nullptr);
    QCOMPARE(anotherTransientPropertyMapping->classPropertyName(), "anotherTransientProperty");
    QCOMPARE(anotherTransientPropertyMapping->tableFieldName(), "anothertransientproperty");
    QCOMPARE(transientPropertyMapping->isObjectId(), false);
    QCOMPARE(transientPropertyMapping->isAutogenerated(), false);
    QCOMPARE(transientPropertyMapping->isTransient(), true);
}

class PersonWithRenamedTown : public Person
{
    Q_OBJECT
    Q_ORM_PROPERTY(town COLUMN townId)

public:
    Q_INVOKABLE PersonWithRenamedTown() {}
};

void MetadataCacheTest::testColumnNameForColumnWithReference()
{
    qRegisterOrmEntity<PersonWithRenamedTown>();

    QOrmMetadataCache cache;
    QOrmMetadata meta = cache.get<PersonWithRenamedTown>();

    auto townPropertyMapping = meta.classPropertyMapping("town");
    QVERIFY(townPropertyMapping != nullptr);
    QCOMPARE(townPropertyMapping->classPropertyName(), "town");
    QCOMPARE(townPropertyMapping->tableFieldName(), "townId");
    QCOMPARE(townPropertyMapping->isReference(), true);
}

void MetadataCacheTest::testNamespacedEntity()
{
    qRegisterOrmEntity<MyNamespace::WithNamespace>();

    QOrmMetadataCache cache;
    QOrmMetadata meta = cache.get<MyNamespace::WithNamespace>();

    QCOMPARE(meta.className(), "MyNamespace::WithNamespace");
    QCOMPARE(meta.tableName(), "MyNamespace_WithNamespace");
}

void MetadataCacheTest::testEnumColumn()
{
    qRegisterOrmEntity<WithEnum>();

    QOrmMetadataCache cache;
    QOrmMetadata meta = cache.get<WithEnum>();

    auto myEnumMapping = meta.classPropertyMapping("myEnum");
    QVERIFY(myEnumMapping != nullptr);
    QCOMPARE(myEnumMapping->classPropertyName(), "myEnum");
    QCOMPARE(myEnumMapping->tableFieldName(), "myenum");
    QCOMPARE(myEnumMapping->isObjectId(), false);
    QCOMPARE(myEnumMapping->isAutogenerated(), false);
    QCOMPARE(myEnumMapping->dataType(), QVariant::Int);
    QCOMPARE(myEnumMapping->dataTypeName(), "MyNamespace::MyEnum");
    QCOMPARE(myEnumMapping->isReference(), false);
    QCOMPARE(myEnumMapping->referencedEntity(), nullptr);
    QCOMPARE(myEnumMapping->isTransient(), false);

    auto myEnumClassMapping = meta.classPropertyMapping("myEnumClass");
    QVERIFY(myEnumClassMapping != nullptr);
    QCOMPARE(myEnumClassMapping->classPropertyName(), "myEnumClass");
    QCOMPARE(myEnumClassMapping->tableFieldName(), "myenumclass");
    QCOMPARE(myEnumClassMapping->isObjectId(), false);
    QCOMPARE(myEnumClassMapping->isAutogenerated(), false);
    QCOMPARE(myEnumClassMapping->dataType(), QVariant::Int);
    QCOMPARE(myEnumClassMapping->dataTypeName(), "MyNamespace::MyEnumClass");
    QCOMPARE(myEnumClassMapping->isReference(), false);
    QCOMPARE(myEnumClassMapping->referencedEntity(), nullptr);
    QCOMPARE(myEnumClassMapping->isTransient(), false);
}

void MetadataCacheTest::testColumnWithNamespacedReference()
{
    qRegisterOrmEntity<WithEnum>();

    QOrmMetadataCache cache;
    QOrmMetadata meta = cache.get<WithEnum>();

    auto myNamespacedClassMapping = meta.classPropertyMapping("myNamespacedClass");
    QVERIFY(myNamespacedClassMapping != nullptr);
    QCOMPARE(myNamespacedClassMapping->classPropertyName(), "myNamespacedClass");
    QCOMPARE(myNamespacedClassMapping->tableFieldName(), "mynamespacedclass_id");
    QCOMPARE(myNamespacedClassMapping->isObjectId(), false);
    QCOMPARE(myNamespacedClassMapping->isAutogenerated(), false);
    QCOMPARE(myNamespacedClassMapping->dataType(), QVariant::UserType);
    QCOMPARE(myNamespacedClassMapping->dataTypeName(), "MyNamespace::WithNamespace*");
    QCOMPARE(myNamespacedClassMapping->isReference(), true);
    QCOMPARE(myNamespacedClassMapping->referencedEntity(),
             &cache.get<MyNamespace::WithNamespace>());
    QCOMPARE(myNamespacedClassMapping->isTransient(), false);
}

QTEST_APPLESS_MAIN(MetadataCacheTest)

#include "tst_metadatacachetest.moc"

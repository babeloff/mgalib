/*** Included Header Files ***/
#include "CoreStorageTesting.h"
#include "../Core/CoreMetaObject.h"


// Initialize the static members outside of the class
CoreProject* ICoreStorageTest::coreProject = NULL;
ICoreStorage* ICoreStorageTest::storage = NULL;
Uuid ICoreStorageTest::atomUuid = Uuid::Null();
Uuid ICoreStorageTest::attributeUuid = Uuid::Null();
//CoreMetaProject* ICoreStorageParamTest::metaProject = NULL;
//CoreProject* ICoreStorageParamTest::coreProject = NULL;
//ICoreStorage* ICoreStorageParamTest::storage = NULL;


// --------------------------- UtilityTests --------------------------- //


TEST(UtilityTests,UuidConversions)
{
	Uuid uuidA;
	// From a Uuid to vector and string
	std::vector<unsigned char> vectorUuidA = uuidA;
	std::string stringUuidA = uuidA;

	// From vector to Uuid and string
	Uuid uuidB = vectorUuidA;
	std::string stringUuidB = Uuid::ToString(vectorUuidA);

	// From string to Uuid and vector
	Uuid uuidC = stringUuidB;
	std::vector<unsigned char> vectorUuidC = Uuid::ToVector(stringUuidB);

	Uuid uuidD;
	EXPECT_NE( uuidA, uuidD );
	EXPECT_EQ( uuidA, uuidB );
	EXPECT_EQ( uuidB, uuidC );
	EXPECT_EQ( stringUuidA, stringUuidB );
	EXPECT_EQ( vectorUuidA, vectorUuidC );
}



// --------------------------- StaticICoreStorage --------------------------- //


TEST(StaticICoreStorageTest,RegisterStorageFactory)
{
	// Try registering a factory with no tag (Expect failure)
	EXPECT_NE( S_OK, ICoreStorage::RegisterStorage("", NULL) )
		<< "Failure on ICoreStorage::RegisterStorage with empty tag.";
	// Try registering a factory with a NULL factory (Expect failure)
	EXPECT_NE( S_OK, ICoreStorage::RegisterStorage("MGA", NULL) )
		<< "Failure on ICoreStorage::RegisterStorage with NULL factory.";
	// Register this storage class (Expect S_OK)
	ASSERT_EQ( S_OK, ICoreStorage::RegisterStorage("MGA", new BinFileFactory()) )
		<< "Failure on ICoreStorage::RegisterStorage - MUST succeed to continue.";
	// Try registering the same storage TAG again (Expect failure)
	EXPECT_NE( S_OK, ICoreStorage::RegisterStorage("MGA", new BinFileFactory()) )
		<< "Failure on ICoreStorage::RegisterStorage with duplicate tag.";

	// Try clearing the factory registry (Expect S_OK)
	EXPECT_EQ( S_OK, ICoreStorage::ClearStorageRegistry() );

	// Try registering the previously registered MGA factory (Expect S_OK)
	ASSERT_EQ( S_OK, ICoreStorage::RegisterStorage("MGA", new BinFileFactory()) )
		<< "Failure on ICoreStorage::RegisterStorage - MUST succeed to continue.";
}


// --------------------------- ICoreStorage Standard --------------------------- //


TEST_F(ICoreStorageTest,CreateObject)
{
	Result_t result;
	Uuid newUuid(Uuid::Null());
	// CreateObject with METAID_NONE (Expect E_INVALID_USAGE)
	EXPECT_EQ( E_INVALID_USAGE, result = storage->CreateObject(METAID_NONE, newUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( Uuid::Null(), newUuid );

	// CreateObject outside of transaction (Expect E_TRANSACTION)
	EXPECT_EQ( E_TRANSACTION, result = storage->CreateObject(DTID_ATOM, newUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( Uuid::Null(), newUuid );

	EXPECT_EQ( S_OK, result = storage->BeginTransaction() )  << GetErrorInfo(result);
	// CreateObject inside transaction with invalid MetaID (Expect E_METAID)
	EXPECT_EQ( E_METAID, result = storage->CreateObject((MGA::MetaID_t)123456, newUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( Uuid::Null(), newUuid );

	// CreateObject inside transaction with valid MetaID (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->CreateObject(DTID_ATOM, newUuid) ) << GetErrorInfo(result);
	// Make sure objID is not OBJID_NONE
	EXPECT_NE( Uuid::Null(), newUuid );
	// Close the object
	EXPECT_EQ( S_OK, result = storage->CloseObject() ) << GetErrorInfo(result);
	// Try opening the object again
	EXPECT_EQ( S_OK, result = storage->OpenObject(newUuid) ) << GetErrorInfo(result);
	// Clean up and exit
	EXPECT_EQ( S_OK, result = storage->CloseObject() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// Start another transaction and see if we can open the object (Expect E_NOTFOUND)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_NOTFOUND, result = storage->OpenObject(newUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// Start another transaction and see if creation persists across commit transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CreateObject(DTID_ATOM, newUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(newUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// Set up the test Uuid for future tests
	ICoreStorageTest::atomUuid = newUuid;
}


TEST_F(ICoreStorageTest,OpenObject)
{
	Result_t result;
	Uuid uuid = Uuid::Null();
	// OpenObject outside of transaction (Expect E_TRANSACTION)
	EXPECT_EQ( E_TRANSACTION, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);

	// OpenObject with transaction but with Uuid == NULL (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction()) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);

	// OpenObject with transaction but with invalid Uuid (Expect E_NOTFOUND)
	uuid = Uuid("{12345678-1234-1234-1234-123412341234}");
	EXPECT_EQ( E_NOTFOUND, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);

	// OpenObject with transaction and valid uuid (Expect S_OK)
	uuid = ICoreStorageTest::atomUuid;
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,CloseObject)
{
	Result_t result;
	Uuid uuid = Uuid::Null();
	// CloseObject outside of transaction (Expect E_TRANSACTION)
	EXPECT_EQ( E_TRANSACTION, result = storage->CloseObject() ) << GetErrorInfo(result);
	
	// CloseObject with transaction but no opened object (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CloseObject() ) << GetErrorInfo(result);

	// CloseObject with transaction and correctly opened object (Expect S_OK)
	uuid = ICoreStorageTest::atomUuid;
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CloseObject() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,DeleteObject)
{
	Result_t result;
	Uuid uuid = Uuid::Null();
	// DeleteObject outside of transaction (Expect E_TRANSACTION)
	EXPECT_EQ( E_TRANSACTION, result = storage->DeleteObject() ) << GetErrorInfo(result);

	EXPECT_EQ( S_OK, result = storage->BeginTransaction() );
	// DeleteObject without valid open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( E_INVALID_USAGE, result = storage->DeleteObject() ) << GetErrorInfo(result);

	uuid = ICoreStorageTest::atomUuid;
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	// DeleteObject with transaction and valid open object (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->DeleteObject() );
	// Make sure there is no open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( E_INVALID_USAGE, result = storage->DeleteObject() ) << GetErrorInfo(result);
	// Make sure we can not open the deleted object
	EXPECT_EQ( E_NOTFOUND, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
/*
	// Make sure objects that were being pointed to no longer have backpointers
	MetaObjIDPair bpPair(101,2);
	EXPECT_EQ( result = storage->OpenObject(bpPair), S_OK ) << GetErrorInfo(result);
	std::list<MetaObjIDPair> bpList;
	EXPECT_EQ( result = storage->GetAttributeValue(ATTRID_PARENT + ATTRID_COLLECTION, bpList), S_OK ) << GetErrorInfo(result);
	std::list<MetaObjIDPair>::iterator bpListIter = bpList.begin();
	while (bpListIter != bpList.end()) {
		// Make sure this pair does not refer to the idPair
		EXPECT_NE( *bpListIter, idPair );
		++bpListIter;
	}
*/
	// Abort the transaction to restore the deleted object
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
	// Make sure we can open the object
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	// Make sure objects that are being pointed to now have backpointers
	// ...
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// Delete the object through commit and test for it being gone
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->DeleteObject() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_NOTFOUND, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,BeginTransaction)
{
	Result_t result;
	// BeginTransaction with no existing transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);

	// BeginTransaction with an outstanding transaction (Expect E_TRANSACTION)
	EXPECT_EQ( E_TRANSACTION, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// BeginTransaction after a commit transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// BeginTransaction after an abort transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,CommitTransaction)
{
	Result_t result;
	// CommitTransaction with no existing transaction (Expect E_TRANSACTION)
	EXPECT_EQ( E_TRANSACTION, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// TODO: Event (Create, Change, Delete) ordering permutations
}


TEST_F(ICoreStorageTest,AbortTransaction)
{
	Result_t result;
	// AbortTransaction with no existing transaction (Expect E_TRANSACTION)
	EXPECT_EQ( E_TRANSACTION, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// TODO: Event (Create, Change, Delete) ordering permutations
}


TEST_F(ICoreStorageTest,LongAttribute)
{
	Uuid uuid(Uuid::Null());
	Result_t result;
	// GetAttributeValue outside of transaction (Expect E_TRANSACTION)
	int32_t value;
	EXPECT_EQ( E_TRANSACTION, result = storage->GetAttributeValue(ATTRID_FCOPARENT, value) ) << GetErrorInfo(result);

	// GetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->GetAttributeValue(ATTRID_FCOPARENT, value) ) << GetErrorInfo(result);

	// GetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->CreateObject(DTID_ATOM, uuid) ) << GetErrorInfo(result);
	ASSERT_NE( uuid, Uuid::Null() );
	ICoreStorageTest::atomUuid = uuid;
	EXPECT_EQ( E_ATTRID, result = storage->GetAttributeValue(12345, value) ) << GetErrorInfo(result);

	// GetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->GetAttributeValue(ATTRID_NAME, value) ) << GetErrorInfo(result);

	// GetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_PERMISSIONS, value) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// SetAttributeValue outside of transaction (Expect E_TRANSACTION)
	int32_t newValue = 34879;
	EXPECT_EQ( E_TRANSACTION, result = storage->SetAttributeValue(ATTRID_FCOPARENT, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->SetAttributeValue(ATTRID_FCOPARENT, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( E_ATTRID, result = storage->SetAttributeValue(12345, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->SetAttributeValue(ATTRID_NAME, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_PERMISSIONS, newValue) ) << GetErrorInfo(result);
	// Test to make sure value was accepted
	int32_t tmpValue;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_PERMISSIONS, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( newValue, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
	// Test to make sure old value was restored
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_PERMISSIONS, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( value, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// SetAttributeValue within a commit transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_PERMISSIONS, newValue) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_PERMISSIONS, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( newValue, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,RealAttribute)
{
	double value;
	Result_t result;
	Uuid uuid(Uuid::Null());
	// GetAttributeValue outside of transaction (Expect E_TRANSACTION)
	EXPECT_EQ( E_TRANSACTION, result = storage->GetAttributeValue(ATTRID_FLOATATTR, value) ) << GetErrorInfo(result);

	// GetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->GetAttributeValue(ATTRID_FLOATATTR, value) ) << GetErrorInfo(result);

	// GetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->CreateObject(DTID_FLOATATTR, uuid) ) << GetErrorInfo(result);
	ASSERT_NE( uuid, Uuid::Null() );
	ICoreStorageTest::attributeUuid = uuid;
	EXPECT_EQ( E_ATTRID, result = storage->GetAttributeValue(12345, value) ) << GetErrorInfo(result);

	// GetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->GetAttributeValue(ATTRID_ATTRPARENT, value) ) << GetErrorInfo(result);

	// GetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_FLOATATTR, value) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// SetAttributeValue outside of transaction (Expect E_TRANSACTION)
	double newValue = 34879.12341;
	EXPECT_EQ( E_TRANSACTION, result = storage->SetAttributeValue(ATTRID_FCOPARENT, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->SetAttributeValue(ATTRID_FCOPARENT, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( E_ATTRID, result = storage->SetAttributeValue(12345, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->SetAttributeValue(ATTRID_ATTRPARENT, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_FLOATATTR, newValue) ) << GetErrorInfo(result);
	// Test to make sure value was accepted
	double tmpValue;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_FLOATATTR, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( newValue, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
	// Test to make sure old value was restored
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_FLOATATTR, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( value, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// SetAttributeValue within a commit transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_FLOATATTR, newValue) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_FLOATATTR, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( newValue, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,StringAttribute)
{
	Result_t result;
	Uuid uuid(Uuid::Null());
	// GetAttributeValue outside of transaction (Expect E_TRANSACTION)
	std::string value;
	EXPECT_EQ( E_TRANSACTION, result = storage->GetAttributeValue(ATTRID_NAME, value) ) << GetErrorInfo(result);
	
	// GetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->GetAttributeValue(ATTRID_NAME, value) ) << GetErrorInfo(result);
	
	// GetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->RootUuid(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( E_ATTRID, result = storage->GetAttributeValue(12345, value) ) << GetErrorInfo(result);
	
	// GetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->GetAttributeValue(ATTRID_PARADIGMUUID, value) ) << GetErrorInfo(result);
	
	// GetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_NAME, value) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// SetAttributeValue outside of transaction (Expect E_TRANSACTION)
	std::string newValue = "A quick brown fox jumps over the lazy dog.";
	EXPECT_EQ( E_TRANSACTION, result = storage->SetAttributeValue(ATTRID_NAME, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->SetAttributeValue(ATTRID_NAME, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( E_ATTRID, result = storage->SetAttributeValue(12345, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->SetAttributeValue(ATTRID_PARADIGMUUID, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_NAME, newValue) ) << GetErrorInfo(result);
	// Test to make sure value was accepted
	std::string tmpValue;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_NAME, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( newValue, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
	// Test to make sure old value was restored
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_NAME, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( value, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// SetAttributeValue within a commit transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_NAME, newValue) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_NAME, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( newValue, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,LongPointerAttribute)
{
	Result_t result;
	Uuid uuid(Uuid::Null());
	// GetAttributeValue outside of transaction (Expect E_TRANSACTION)
	Uuid value;
	EXPECT_EQ( E_TRANSACTION, result = storage->GetAttributeValue(ATTRID_FLOATATTR, value) ) << GetErrorInfo(result);
	
	// GetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->GetAttributeValue(ATTRID_FLOATATTR, value) ) << GetErrorInfo(result);

	// GetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->RootUuid(uuid) ) << GetErrorInfo(result) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( E_ATTRID, result = storage->GetAttributeValue(12345, value) ) << GetErrorInfo(result);

	// GetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->GetAttributeValue(ATTRID_NAME, value) ) << GetErrorInfo(result);
	
	// GetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_PARADIGMUUID, value) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
	
	// SetAttributeValue outside of transaction (Expect E_TRANSACTION)
	Uuid newValue;
	EXPECT_EQ( E_TRANSACTION, result = storage->SetAttributeValue(ATTRID_FLOATATTR, newValue) ) << GetErrorInfo(result);
	
	// SetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->SetAttributeValue(ATTRID_FLOATATTR, newValue) ) << GetErrorInfo(result);
	
	// SetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( E_ATTRID, result = storage->SetAttributeValue(12345, newValue) ) << GetErrorInfo(result);
	
	// SetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->SetAttributeValue(ATTRID_NAME, newValue) ) << GetErrorInfo(result);
	
	// SetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_PARADIGMUUID, newValue) ) << GetErrorInfo(result);
	// Test to make sure value was accepted
	Uuid tmpValue;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_PARADIGMUUID, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( newValue, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
	// Test to make sure old value was restored
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_PARADIGMUUID, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( value, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
	
	// SetAttributeValue within a commit transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_PARADIGMUUID, newValue) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_PARADIGMUUID, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( newValue, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,PointerAttribute)
{
	Result_t result;
	Uuid attributeUuid = ICoreStorageTest::attributeUuid;
	// GetAttributeValue outside of transaction (Expect E_TRANSACTION)
	Uuid value(Uuid::Null());
	EXPECT_EQ( E_TRANSACTION, result = storage->GetAttributeValue(ATTRID_ATTRPARENT, value) ) << GetErrorInfo(result);

	// GetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->GetAttributeValue(ATTRID_ATTRPARENT, value) ) << GetErrorInfo(result);
	
	// Open an object (a FloatAttribute) we know has a pointer attribute
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	// Get a non-existant pointer attribute value (Foo attribute) (Expect E_ATTRID)
	EXPECT_EQ( E_ATTRID, result = storage->GetAttributeValue(123, value) ) << GetErrorInfo(result);

	// Get an attribute that isn't a pointer (ATTRID_FLOATATTR) (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->GetAttributeValue(ATTRID_FLOATATTR, value) ) << GetErrorInfo(result);

	// Get the pointer attribute value (ATTRID_ATTRPARENT) (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_ATTRPARENT, value) ) << GetErrorInfo(result);
	EXPECT_EQ( Uuid::Null(), value );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// SetAttributeValue outside of transaction (Expect E_TRANSACTION)
	Uuid atomUuid = ICoreStorageTest::atomUuid;
	EXPECT_EQ( E_TRANSACTION, result = storage->SetAttributeValue(ATTRID_ATTRPARENT, atomUuid) ) << GetErrorInfo(result);
	
	// SetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->SetAttributeValue(ATTRID_ATTRPARENT, atomUuid) ) << GetErrorInfo(result);

	// SetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( E_ATTRID, result = storage->SetAttributeValue(12345, atomUuid) ) << GetErrorInfo(result);
	
	// SetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->SetAttributeValue(ATTRID_FLOATATTR, atomUuid) ) << GetErrorInfo(result);

	// SetAttributeValue with everything corret (Float attribute now points to atom as parent) (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_ATTRPARENT, atomUuid) ) << GetErrorInfo(result);
	// Test to make sure value was accepted
	Uuid tmpValue;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_ATTRPARENT, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( atomUuid, tmpValue );
	// Make sure object now being pointed to has backpointer
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	// Get backpointer list (ATTRID_ATTRPARENT) and make sure it has uuid in it
	std::list<Uuid> backpointers;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_ATTRPARENT+ATTRID_COLLECTION, backpointers) ) << GetErrorInfo(result);
	EXPECT_EQ( 1, backpointers.size() );
	EXPECT_EQ( attributeUuid, backpointers.front() );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// Test to make sure old values (both forward and backpointer) were restored
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_ATTRPARENT, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( value, tmpValue );
	// Make sure object that was being pointed to no longer has backpointer
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	// Get backpointer list (ATTRID_ATTRPARENT) and make sure it is empty
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_ATTRPARENT+ATTRID_COLLECTION, backpointers) ) << GetErrorInfo(result);
	EXPECT_EQ( 0, backpointers.size() );
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	
	// SetAttributeValue within a commit transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_ATTRPARENT, atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_ATTRPARENT, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( atomUuid, tmpValue );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// Clean up the atom's connection to the root object
	Uuid rootUuid;
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->RootUuid(rootUuid)  ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_FCOPARENT, rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,DictionaryAttribute)
{
	Uuid uuid(Uuid::Null());
	Result_t result;
	// GetAttributeValue outside of transaction (Expect E_TRANSACTION)
	DictionaryMap value;
	EXPECT_EQ( E_TRANSACTION, result = storage->GetAttributeValue(ATTRID_REGISTRY, value) ) << GetErrorInfo(result);
	
	// GetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->GetAttributeValue(ATTRID_REGISTRY, value) ) << GetErrorInfo(result);
	
	// GetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	Uuid atomUuid = ICoreStorageTest::atomUuid;
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( E_ATTRID, result = storage->GetAttributeValue(12345, value) ) << GetErrorInfo(result);
	
	// GetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->GetAttributeValue(ATTRID_NAME, value) ) << GetErrorInfo(result);

	// GetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_REGISTRY, value) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	// Dictionary should be empty at this point
	EXPECT_EQ( 0, value.size() );

	// SetAttributeValue outside of transaction (Expect E_TRANSACTION)
	DictionaryMap newValue;
	newValue.insert( std::make_pair("TestKey", "Test Value") );
	EXPECT_EQ( E_TRANSACTION, result = storage->SetAttributeValue(ATTRID_REGISTRY, newValue) ) << GetErrorInfo(result);
	
	// SetAttributeValue within transaction without open object (Expect E_INVALID_USAGE)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( E_INVALID_USAGE, result = storage->SetAttributeValue(ATTRID_REGISTRY, newValue) ) << GetErrorInfo(result);
	
	// SetAttributeValue with transaction and with open object but invalid AttrID (Expect E_ATTRID)
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( E_ATTRID, result = storage->SetAttributeValue(12345, newValue) ) << GetErrorInfo(result);
	
	// SetAttributeValue with transaction, open object and valid AttrID, but AttrID does not match type (Expect E_ATTVALTYPE)
	EXPECT_EQ( E_ATTVALTYPE, result = storage->SetAttributeValue(ATTRID_NAME, newValue) ) << GetErrorInfo(result);

	// SetAttributeValue with everything corret (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_REGISTRY, newValue) ) << GetErrorInfo(result);
	// Test to make sure value was accepted
	DictionaryMap tmpValue;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_REGISTRY, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( 1, tmpValue.size() );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);
	// Test to make sure old value was restored
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_REGISTRY, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( 0, tmpValue.size() );
	EXPECT_EQ( S_OK, result = storage->AbortTransaction() ) << GetErrorInfo(result);

	// SetAttributeValue within a commit transaction (Expect S_OK)
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_REGISTRY, newValue) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_REGISTRY, tmpValue) ) << GetErrorInfo(result);
	EXPECT_EQ( newValue.size(), tmpValue.size() );
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,Save)
{
	Result_t result;
	// Save with "" (existing filename) as filename
	ASSERT_EQ( S_OK, result = storage->Save("") ) << GetErrorInfo(result);
	// Make sure the three objects are there and correct
	Uuid rootUuid(Uuid::Null());
	EXPECT_EQ( S_OK, result = storage->RootUuid(rootUuid) ) << GetErrorInfo(result);
	std::vector<Uuid> objectVector;
	EXPECT_EQ( S_OK, result = storage->ObjectVector(objectVector) ) << GetErrorInfo(result);
	EXPECT_EQ( 3, objectVector.size() );
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	//Save with simple filename (no directory path)
	ASSERT_EQ( S_OK, result = storage->Save("testOrama.mga", true) ) << GetErrorInfo(result);
	// Make sure the objects are there and correct
	EXPECT_EQ( S_OK, result = storage->ObjectVector(objectVector) ) << GetErrorInfo(result);
	EXPECT_EQ( 3, objectVector.size() );
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	Uuid atomParent = Uuid::Null();
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_FCOPARENT, atomParent) ) << GetErrorInfo(result);
	EXPECT_EQ( atomParent, rootUuid );
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// Save with full path (directory + filename) from simple path
	ASSERT_EQ( S_OK, result = storage->Save("Subfolder/testOrama2.mga", true) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	Uuid attributeParent = Uuid::Null();
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_ATTRPARENT, attributeParent) ) << GetErrorInfo(result);
	EXPECT_EQ( attributeParent, atomUuid );
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// Save from full path (directory + filename) to simple path
	ASSERT_EQ( S_OK, result = storage->Save("testOrama3.mga", true) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	std::string rootName;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_NAME, rootName) ) << GetErrorInfo(result);
	EXPECT_STREQ( rootName.c_str(), "A quick brown fox jumps over the lazy dog." ); 
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// Clean up all of the saves
	ASSERT_EQ( S_OK, result = storage->Save("tmpfile.mga", true) ) << GetErrorInfo(result);
	EXPECT_EQ( 0, remove("testOrama.mga") );
	EXPECT_EQ( 0, remove("Subfolder/testOrama2.mga") );
	EXPECT_EQ( 0, remove("testOrama3.mga") );

	// Make sure the objects are there and correct
	EXPECT_EQ( S_OK, result = storage->ObjectVector(objectVector) ) << GetErrorInfo(result);
	EXPECT_EQ( 3, objectVector.size() );
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	double floatValue = 8475734.3784;
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_FLOATATTR, floatValue) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,CacheSize)
{
	Result_t result;
	Uuid rootUuid(Uuid::Null());
	EXPECT_EQ( S_OK, result = storage->RootUuid(rootUuid) ) << GetErrorInfo(result);
	// Set cacheSize == 1
	EXPECT_EQ( S_OK, result = storage->SetCacheSize(1) ) << GetErrorInfo(result);
	uint64_t cacheSize;
	EXPECT_EQ( S_OK, result = storage->GetCacheSize(cacheSize) ) << GetErrorInfo(result);
	EXPECT_EQ( cacheSize, 1 );

	// Perform simple lookups to try putting more objects into the cache than 1
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	// Try to set cache size during a transaction (Expect E_TRANSACTION)
	EXPECT_EQ( E_TRANSACTION, result = storage->SetCacheSize(8474) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetCacheSize(cacheSize) ) << GetErrorInfo(result);
	EXPECT_EQ( cacheSize, 1 );
	EXPECT_EQ( S_OK, result = storage->CommitTransaction()) << GetErrorInfo(result);

	// Perform simple lookups with cacheSize == 2
	EXPECT_EQ( S_OK, result = storage->SetCacheSize(2) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetCacheSize(cacheSize) ) << GetErrorInfo(result);
	EXPECT_EQ( cacheSize, 1 );
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->GetCacheSize(cacheSize) ) << GetErrorInfo(result);
	EXPECT_EQ( cacheSize, 2 );
	EXPECT_EQ( S_OK, result = storage->CommitTransaction()) << GetErrorInfo(result);

	// Make changes to two of the three objects with cache size = 1
	EXPECT_EQ( S_OK, result = storage->SetCacheSize(1) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	std::string atomName = "I am a happy atom.";
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_NAME, atomName) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	double floatValue = 4783823.45934;
	EXPECT_EQ( S_OK, result = storage->SetAttributeValue(ATTRID_FLOATATTR, floatValue) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// Save with objects in cache and scratch file (from being pushed out of queue) and changes
	ASSERT_EQ( S_OK, result = storage->Save("tmpfile.mga") ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,Compression)
{
	Result_t result;
	Uuid rootUuid(Uuid::Null());
	EXPECT_EQ( S_OK, result = storage->RootUuid(rootUuid) ) << GetErrorInfo(result);
	// First decompress, test and save
	EXPECT_EQ( S_OK, result = storage->DisableCompression() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	ASSERT_EQ( S_OK, result = storage->Save("Subfolder/tmpfile_uncompressed.mga", true) ) << GetErrorInfo(result);
	// Then recompress, test and save
	EXPECT_EQ( S_OK, result = storage->EnableCompression() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	ASSERT_EQ( S_OK, result = storage->Save("tmpfile_duplicate.mga", true) ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,Encryption)
{
	Result_t result;
	Uuid rootUuid(Uuid::Null());
	EXPECT_EQ( S_OK, result = storage->RootUuid(rootUuid) ) << GetErrorInfo(result);
	// First encrypt, test and save
//	EXPECT_EQ( S_OK, result = storage->EnableEncryption() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	ASSERT_EQ( S_OK, result = storage->Save("tmpfile_encrypted.mga", true) ) << GetErrorInfo(result);
	// Then decrypt, test and save
	EXPECT_EQ( S_OK, result = storage->DisableEncryption() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	ASSERT_EQ( S_OK, result = storage->Save("tmpfile.mga", true) ) << GetErrorInfo(result);
}


TEST_F(ICoreStorageTest,Open)
{
	Result_t result;
	CoreMetaProject	*coreMetaProject = NULL;
	EXPECT_EQ( S_OK, result = CreateMGACoreMetaProject(coreMetaProject)) << GetErrorInfo(result);
	// Open with unknown tag (Expect E_UNKNOWN_STORAGE)
	ICoreStorage* openStorage = NULL;
	EXPECT_EQ( E_UNKNOWN_STORAGE, result = ICoreStorage::Open("ASD", "foo.mga", NULL, openStorage) ) << GetErrorInfo(result);
	// Open with known tag, but empty filename (Expect E_NAMEMISSING)
	EXPECT_EQ( E_NAMEMISSING, result = ICoreStorage::Open("MGA", "", NULL, openStorage) ) << GetErrorInfo(result);
	// Open with good tag but unknown file (Expect E_FILEOPEN)
	EXPECT_EQ( E_FILEOPEN, result = ICoreStorage::Open("MGA", "asdfg.mga", coreMetaProject, openStorage) ) << GetErrorInfo(result);
	// Open with good tag but invalid CoreMetaProject (Expect E_META_NOTOPEN)
	EXPECT_EQ( E_META_NOTOPEN, result = ICoreStorage::Open("MGA", "asdfg.mga", NULL, openStorage) ) << GetErrorInfo(result);

	// Open with good tag, CoreMetaProject and filename (Expect S_OK)
	ASSERT_EQ( S_OK, result = ICoreStorage::Open("MGA", "tmpfile_duplicate.mga", coreMetaProject, openStorage) ) << GetErrorInfo(result);
	ASSERT_TRUE( openStorage != NULL );
	// Do some reads and writes for fun
	EXPECT_EQ( S_OK, result = openStorage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	Uuid rootUuid(Uuid::Null());
	EXPECT_EQ( S_OK, result = openStorage->RootUuid(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->CommitTransaction() ) << GetErrorInfo(result);
	delete openStorage;
	openStorage = NULL;
	EXPECT_EQ( 0, remove("tmpfile_duplicate.mga") );

	// Open with full path (directory + filename) and how about uncompressed
	ASSERT_EQ( S_OK, result = ICoreStorage::Open("MGA", "Subfolder/tmpfile_uncompressed.mga", coreMetaProject, openStorage) ) << GetErrorInfo(result);
	ASSERT_TRUE( openStorage != NULL );
	EXPECT_EQ( S_OK, result = openStorage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->CommitTransaction() ) << GetErrorInfo(result);
	delete openStorage;
	openStorage = NULL;
	EXPECT_EQ( 0, remove("Subfolder/tmpfile_uncompressed.mga") );

	// Open with simple path and how about encrypted
	ASSERT_EQ( S_OK, result = ICoreStorage::Open("MGA", "tmpfile_encrypted.mga", coreMetaProject, openStorage) ) << GetErrorInfo(result);
	ASSERT_TRUE( openStorage != NULL );
	EXPECT_EQ( S_OK, result = openStorage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->OpenObject(rootUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = openStorage->CommitTransaction() ) << GetErrorInfo(result);
	delete openStorage;
	delete coreMetaProject;
	openStorage = NULL;
	EXPECT_EQ( 0, remove("tmpfile_encrypted.mga") );
}


TEST_F(ICoreStorageTest,BasicUndo)
{
	Result_t result;
	Uuid rootUuid(Uuid::Null());
	EXPECT_EQ( S_OK, result = storage->RootUuid(rootUuid) ) << GetErrorInfo(result);
	// Get base-line undo/redo counts
	uint64_t undoCount = 0, redoCount = 99;
	EXPECT_EQ( S_OK, result = storage->UndoCount(undoCount) ) << GetErrorInfo(result);
	EXPECT_EQ( 13, undoCount );
	EXPECT_EQ( S_OK, result = storage->RedoCount(redoCount) ) << GetErrorInfo(result);
	EXPECT_EQ( 0, redoCount );

	// Undo the last transaction
	Uuid undoTag = Uuid::Null();
	EXPECT_EQ( S_OK, result = storage->Undo(undoTag) ) << GetErrorInfo(result);
	// Make sure the value got undone
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	std::string atomName = "This is a test.";
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_NAME, atomName) ) << GetErrorInfo(result);
	EXPECT_STREQ( "", atomName.c_str() );
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	double floatValue = 0.0;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_FLOATATTR, floatValue) ) << GetErrorInfo(result);
	EXPECT_EQ( 8475734.3784, floatValue );
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
	
	// Make sure the undo/redo counts are accurate
	EXPECT_EQ( S_OK, result = storage->UndoCount(undoCount) ) << GetErrorInfo(result);
	EXPECT_EQ( 12, undoCount );
	EXPECT_EQ( S_OK, result = storage->RedoCount(redoCount) ) << GetErrorInfo(result);
	EXPECT_EQ( 1, redoCount );
}


TEST_F(ICoreStorageTest,BasicRedo)
{
	Result_t result;
	Uuid rootUuid(Uuid::Null());
	EXPECT_EQ( S_OK, result = storage->RootUuid(rootUuid) ) << GetErrorInfo(result);
	Uuid undoTag = Uuid::Null();

	// Redo the last transaction
	uint64_t undoCount = 0, redoCount = 99;
	EXPECT_EQ( S_OK, result = storage->Redo(undoTag) ) << GetErrorInfo(result);
	// Make sure the value got undone
	EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->OpenObject(atomUuid) ) << GetErrorInfo(result);
	std::string atomName = "Once upon a time.";
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_NAME, atomName) ) << GetErrorInfo(result);
	EXPECT_STREQ( "I am a happy atom.", atomName.c_str() );
	EXPECT_EQ( S_OK, result = storage->OpenObject(attributeUuid) ) << GetErrorInfo(result);
	double floatValue = 0.0;
	EXPECT_EQ( S_OK, result = storage->GetAttributeValue(ATTRID_FLOATATTR, floatValue) ) << GetErrorInfo(result);
	EXPECT_EQ( 4783823.45934, floatValue );
	EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);

	// Make sure the undo/redo counts are accurate
	EXPECT_EQ( S_OK, result = storage->UndoCount(undoCount) ) << GetErrorInfo(result);
	EXPECT_EQ( 13, undoCount );
	EXPECT_EQ( S_OK, result = storage->RedoCount(redoCount) ) << GetErrorInfo(result);
	EXPECT_EQ( 0, redoCount );
}



// --------------------------- ICoreStorage Hammer Test --------------------------- //


static inline void _HammerCreateObject(const bool &isLogging, std::ostream &out, ICoreStorage* storage, CoreMetaProject *coreMetaProject,
									   std::tr1::unordered_map<Uuid,void*,Uuid_HashFunc> &objectMap)
{
	Result_t result;
	// Get the list of possible objects to create
	std::list<CoreMetaObject*> objectList;
	EXPECT_EQ( S_OK, coreMetaProject->GetObjects(objectList) ) << GetErrorInfo(result);
	// Choose one from the list
	int selection = rand() % objectList.size();
	std::list<CoreMetaObject*>::iterator objectIter = objectList.begin();
	for (int i=0; i<selection; i++) ++objectIter;

	// What is the meta ID?
	MetaID_t metaID;
	EXPECT_EQ( S_OK, (*objectIter)->GetMetaID(metaID) ) << GetErrorInfo(result);
	Uuid uuid = Uuid::Null();
	// Grok some output
	if (isLogging) out << "\t\tstorage->CreateObject(" << metaID << ", uuid);\n";
	EXPECT_EQ( S_OK, result = storage->CreateObject(metaID, uuid) ) << GetErrorInfo(result);
	EXPECT_NE( Uuid::Null(), uuid );
	objectMap.insert( std::make_pair(uuid,(void*)NULL) );
}

static inline void _HammerChangeAttribute(const bool &isLogging, std::ostream &out, ICoreStorage* storage, CoreMetaProject *coreMetaProject,
										  std::tr1::unordered_map<Uuid,void*,Uuid_HashFunc> &objectMap)
{
	Result_t result;
	if (objectMap.size() == 0) return;
	// Choose one from the map
	int selection = rand() % objectMap.size();
	std::tr1::unordered_map<Uuid,void*,Uuid_HashFunc>::iterator objectIter = objectMap.begin();
	for (int i=0; i<selection; i++) ++objectIter;
	Uuid uuid = objectIter->first;
	
	// What attributes does this object have
	EXPECT_EQ( S_OK, result = storage->OpenObject(uuid) ) << GetErrorInfo(result);
	CoreMetaObject *metaObject = NULL;
	EXPECT_EQ( S_OK, result = storage->MetaObject(metaObject) ) << GetErrorInfo(result);
	EXPECT_TRUE( metaObject != NULL );
	std::list<CoreMetaAttribute*> attribList;
	EXPECT_EQ( S_OK, metaObject->GetAttributes(attribList) ) << GetErrorInfo(result);
	// Choose one to change
	selection = rand() % attribList.size();
	std::list<CoreMetaAttribute*>::iterator attribIter = attribList.begin();
	for (int i=0; i<selection; i++) ++attribIter;
	CoreMetaAttribute* coreMetaAttribute = *attribIter;
	// Finally, get the AttrID_t and ValueType for this attribute
	AttrID_t attrID = ATTRID_NONE;
	ValueType valueType;
	EXPECT_EQ( S_OK, coreMetaAttribute->GetAttributeID(attrID) ) << GetErrorInfo(result);
	EXPECT_NE( ATTRID_NONE, attrID );
	EXPECT_EQ( S_OK, coreMetaAttribute->GetValueType(valueType) ) << GetErrorInfo(result);
	EXPECT_NE( ValueType::None(), valueType );

	// Now make the change based on type
	if (valueType == ValueType::Long())
	{
		int32_t longValue = rand();
		if (isLogging) out << "\t\tstorage->SetAttributeValue(" << attrID << ", " << longValue << ");\n";
		EXPECT_EQ( S_OK, storage->SetAttributeValue(attrID, longValue) ) << GetErrorInfo(result);
	}
	else if (valueType == ValueType::Real())
	{
		double realValue = (double)(rand() - rand()) / (double)rand();
		if (isLogging) out << "\t\tstorage->SetAttributeValue(" << attrID << ", " << realValue << ");\n";
		EXPECT_EQ( S_OK, storage->SetAttributeValue(attrID, realValue) ) << GetErrorInfo(result);
	}
	else if (valueType == ValueType::String())
	{
		std::string stringValue("random string here");
		if (isLogging) out << "\t\tstorage->SetAttributeValue(" << attrID << ", \"" << stringValue << "\");\n";
		EXPECT_EQ( S_OK, storage->SetAttributeValue(attrID, stringValue) ) << GetErrorInfo(result);
	}
	else if (valueType == ValueType::LongPointer())
	{
		Uuid longpointerValue;
		if (isLogging) out << "\t\tstorage->SetAttributeValue(" << attrID << ", \"" << longpointerValue << "\");\n";
		EXPECT_EQ( S_OK, storage->SetAttributeValue(attrID, longpointerValue) ) << GetErrorInfo(result);
	}
	else if (valueType == ValueType::Pointer())
	{
		if (isLogging) out << "\t\t// Comment taking the place of a pointer SetAttribute.\n";
//		Uuid pointerValue;
//		EXPECT_EQ( S_OK, storage->SetAttributeValue(attrID, pointerValue) ) << GetErrorInfo(result);
//		out << "\tstorage->SetAttributeValue(attrID, \"" << pointerValue << "\");\n";		
	}
	else if (valueType == ValueType::Dictionary())
	{
		DictionaryMap dictionaryValue;
		EXPECT_EQ( S_OK, storage->GetAttributeValue(attrID, dictionaryValue) ) << GetErrorInfo(result);
		std::string key("randomKey"), value("randomValue");
		dictionaryValue.insert( std::make_pair(key, value) );
		if (isLogging) out << "\tstorage->SetAttributeValue(" << attrID << ", dictionaryValue);\n";
		EXPECT_EQ( S_OK, storage->SetAttributeValue(attrID, dictionaryValue) ) << GetErrorInfo(result);
	}
	// Can't do anything about a Collection type...
}

static inline void _HammerDeleteObject(const bool &isLogging, std::ostream &out, ICoreStorage* storage, CoreMetaProject *coreMetaProject,
									   std::tr1::unordered_map<Uuid,void*,Uuid_HashFunc> &objectMap)
{
	Result_t result;
	if (objectMap.size() == 0) return;
	// Choose one from the list
	int selection = rand() % objectMap.size();
	std::tr1::unordered_map<Uuid,void*,Uuid_HashFunc>::iterator objectIter = objectMap.begin();
	for (int i=0; i<selection; i++) ++objectIter;

	// Make sure this is not the root object
	Uuid rootUuid;
	EXPECT_EQ( S_OK, storage->RootUuid(rootUuid) ) << GetErrorInfo(result);
	if (rootUuid == objectIter->first) return;

	// Grok some output
	if (isLogging) out << "\t\tstorage->OpenObject(\"" << objectIter->first << "\");\n";
	if (isLogging) out << "\t\tstorage->DeleteObject();\n";
	// Ok, open and delete the object
	EXPECT_EQ( S_OK, result = storage->OpenObject(objectIter->first) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->DeleteObject() ) << GetErrorInfo(result);
	// Remove the item from the map
	objectMap.erase(objectIter->first);
}

static inline void _HammerSave(const bool &isLogging, std::ostream &out, ICoreStorage* storage, CoreMetaProject *coreMetaProject)
{
	Result_t result;
	if (isLogging) out << "\tstorage->Save(\"hammer_test.mga\", true);\n";
	EXPECT_EQ( S_OK, result = storage->Save("hammer_test.mga", true) ) << GetErrorInfo(result);
}

static inline void _HammerOpen(const bool &isLogging, std::ostream &out, ICoreStorage* &storage, CoreMetaProject* &coreMetaProject, CoreProject* &coreProject)
{
	Result_t result;
	// First, delete the coreProject (this will loose all transactions since the last save)
	delete coreProject;
	if (isLogging) out << "\tdelete coreProject;\n";

	// Create a new CoreMetaProject
	EXPECT_EQ( S_OK, result = CreateMGACoreMetaProject(coreMetaProject) ) << GetErrorInfo(result);
	ASSERT_TRUE( coreMetaProject != NULL );

	// Now open the file
	EXPECT_EQ( S_OK, result = CoreProject::Open("MGA=hammer_test.mga", coreMetaProject, coreProject) ) << GetErrorInfo(result);
	if (isLogging) out << "\tCoreProject::Open(\"MGA=hammer_test.mga\", metaProject, coreProject);\n";

	// And grab the ICoreStorage pointer
	EXPECT_EQ( S_OK, result = coreProject->Storage(storage) ) << GetErrorInfo(result);
	if (isLogging) out << "\tcoreProject->Storage(storage);\n";
}

static inline void _HammerUndo(const bool &isLogging, std::ostream &out, ICoreStorage* storage, CoreMetaProject *coreMetaProject)
{
	// How many undos can we do
	Result_t result;
	uint64_t undoCount;
	EXPECT_EQ( S_OK, result = storage->UndoCount(undoCount) ) << GetErrorInfo(result);
	if (undoCount == 0) return;
	// And, how many will we undo
	undoCount = rand() % undoCount;
	// Engage...
//	for (uint32_t i=0; i < undoCount; i++)
//	{
		if (isLogging) out << "\tstorage->Undo();\n";
		Uuid tag;
		EXPECT_EQ( S_OK, result = storage->Undo(tag) ) << GetErrorInfo(result);
//	}
}

static inline void _HammerRedo(const bool &isLogging, std::ostream &out, ICoreStorage* storage, CoreMetaProject *coreMetaProject)
{
	// How many undos can we do
	Result_t result;
	uint64_t redoCount;
	EXPECT_EQ( S_OK, result = storage->RedoCount(redoCount) ) << GetErrorInfo(result);
	if (redoCount == 0) return;
	// And, how many will we redo
	redoCount = rand() % redoCount;
	// Engage...
	//	for (uint32_t i=0; i < undoCount; i++)
//	{
		if (isLogging) out << "\tstorage->Redo();\n";
		Uuid tag;
		EXPECT_EQ( S_OK, result = storage->Redo(tag) ) << GetErrorInfo(result);
//	}
}

TEST(ICoreStorage,Hammer)
{
	// Are we running any hammer tests?
	if (hammerTestSize == 0) return;
	std::ostream &out = std::cout;
//	std::ofstream out("hammer_log.log");
	bool isLogging = false;

	// Prepare the new output file
	Result_t result;
	// Get everything up to storage ready
	CoreMetaProject	*coreMetaProject = NULL;
	EXPECT_EQ( S_OK, result = CreateMGACoreMetaProject(coreMetaProject) ) << GetErrorInfo(result);
	ASSERT_TRUE( coreMetaProject != NULL );
	// Create a new test coreProject
	CoreProject* coreProject = NULL;
	EXPECT_EQ( S_OK, result = CoreProject::Create("MGA=hammer_test.mga", coreMetaProject, coreProject) ) << GetErrorInfo(result);
	ASSERT_TRUE( coreProject != NULL );
	// Get the ICoreStorage pointer
	ICoreStorage* storage = NULL;
	EXPECT_EQ( S_OK, result = coreProject->Storage(storage) ) << GetErrorInfo(result);
	ASSERT_TRUE( storage != NULL );
	// Save the project (just to make sure in case an Open is called before the first Save)
	EXPECT_EQ( S_OK, result = storage->Save("hammer_test.mga", true) ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->DisableCaching() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->DisableJournaling() ) << GetErrorInfo(result);
	EXPECT_EQ( S_OK, result = storage->DisableCompression() ) << GetErrorInfo(result);
	// Prep the object hash
	std::tr1::unordered_map<Uuid,void*,Uuid_HashFunc> objectMap;

	std::cout << "------ Commencing hammer test.  Size: " << hammerTestSize << " ------\n";
	// Initialize random seed
	srand ( (unsigned int)time(NULL) );
	// Loop through the test  int iSecret, iGuess;
	for (uint32_t hammerCount=0; hammerCount < hammerTestSize; hammerCount++)
	{
		// Choose an action at random (0-12)
		int selection = rand() % 13;
		switch (selection)
		{
			case 0:	case 1: case 2: case 3: case 4: case 5: case 6: case 7:
			{	// Transaction selected.  How many items in the transaction
				if (isLogging) out << "\tstorage->BeginTransaction();\n";
				int txItems = rand() % 10 + 1;
				EXPECT_EQ( S_OK, result = storage->BeginTransaction() ) << GetErrorInfo(result);
				for (int txCount=0; txCount < txItems; txCount++)
				{
					int subSelection = rand() % 6;
					switch (subSelection)
					{
						case 0: case 1: case 2:
							// Change an attribute
							_HammerChangeAttribute(isLogging, out, storage, coreMetaProject, objectMap);
							break;
						case 3: case 4:
							// Create an object
							_HammerCreateObject(isLogging, out, storage, coreMetaProject, objectMap);
							break;
						case 5:
							// Delete an object
							_HammerDeleteObject(isLogging, out, storage, coreMetaProject, objectMap);
							break;
					}
				}
				EXPECT_EQ( S_OK, result = storage->CommitTransaction() ) << GetErrorInfo(result);
				if (isLogging) out << "\tstorage->CommitTransaction();\n";
			}
				break;
			case 8: case 9:
				// Save the file
//				_HammerSave(isLogging, out, storage, coreMetaProject);
				break;
			case 10:
				// Close the file (without saving changes) and reopen - will loose changes since last save
//				_HammerOpen(isLogging, out, storage, coreMetaProject, coreProject);
				break;
			case 12:
				// Undo the last n changes
				_HammerUndo(isLogging, out, storage, coreMetaProject);
				break;
			case 11:
				// Redo the last n undone changes
				_HammerRedo(isLogging, out, storage, coreMetaProject);
				break;
			default:
				break;
		}
	}

	// Clean up after the hammer test
	EXPECT_EQ( S_OK, result = coreProject->Save("hammer_test.mga", true) ) << GetErrorInfo(result);

	// Do some reporting
	std::cout << "Hammer Test Report...\n";
	std::vector<Uuid> objectVector;
	storage->ObjectVector(objectVector);
	std::cout << "\tObjects: " << objectVector.size() << std::endl;
	uint64_t undoCount, redoCount;
	storage->UndoCount(undoCount);
	std::cout << "\tUndo Size: " << undoCount << std::endl;
	storage->RedoCount(redoCount);
	std::cout << "\tRedo Size: " << redoCount << std::endl;

	delete coreProject;
	EXPECT_EQ( 0, remove("hammer_test.mga") );
}


// --------------------------- ICoreStorage Parameterized --------------------------- //

/*
TEST_P(ICoreStorageParamTest,MetaObject)
{
	CoreMetaObject* coreMetaObject = NULL;

	// Get CoreMetaObject outside of transaction (Expect E_TRANSACTION)
	result = storage->MetaObject(coreMetaObject);
	EXPECT_EQ(result, E_TRANSACTION);

	// Get CoreMetaObject without open object (Expect E_INVALID_USAGE)
	result = storage->BeginTransaction();
	EXPECT_EQ(result, S_OK);
	result = storage->MetaObject(coreMetaObject);
	EXPECT_EQ(result, E_INVALID_USAGE);

	// Open an object then try to get its CoreMetaObject (Expect S_OK)
	result = storage->OpenObject(idPair);
	EXPECT_EQ(result, S_OK);
	result = storage->MetaObject(coreMetaObject);
	EXPECT_EQ(result, S_OK);
	MetaID_t tmpMetaID = METAID_NONE;
	EXPECT_EQ(coreMetaObject->MetaID(tmpMetaID), S_OK);
	EXPECT_EQ(tmpMetaID, idPair.metaID);

	// Abort the transaction to clean up
	result = storage->AbortTransaction();
	EXPECT_EQ(result, S_OK);	
}


TEST_P(ICoreStorageParamTest,MetaID)
{
	MetaID_t metaID = METAID_NONE;

	// Get metaID outside of transaction (Expect E_TRANSACTION)
	result = storage->MetaID(metaID);
	EXPECT_EQ(result, E_TRANSACTION);
}


TEST_P(ICoreStorageParamTest,AttributeChangeWithAbort)
{
	int32_t longValue, longValueAlt=457868;
	double realValue, realValueAlt=483.485786;
	std::string stringValue, stringValueAlt="asdkfjhaslkdjfh";
	std::vector<unsigned char> binaryValue, binaryValueAlt;
	std::list<MetaObjIDPair> collectionValue, collectionValueAlt;
	MetaObjIDPair pointerValue, pointerValueAlt;

	// Test every attribute in the object (via its meta attribute list)
	std::list<CoreMetaAttribute*>::iterator metaAttributeIter = attributeList.begin();
	while (metaAttributeIter != attributeList.end())
	{
		AttrID_t attrID;
		ASSERT_EQ((*metaAttributeIter)->AttributeID(attrID), S_OK );
		ValueType attrType = ValueType::None();
		ASSERT_EQ((*metaAttributeIter)->GetValueType(attrType), S_OK);

		// GetAttributeValue(long) outside of transaction (Expect E_TRANSACTION)
		result = storage->GetAttributeValue(attrID, longValue);
		EXPECT_EQ(result, E_TRANSACTION);
		// GetAttributeValue(real) outside of transaction (Expect E_TRANSACTION)
		result = storage->GetAttributeValue(attrID, realValue);
		EXPECT_EQ(result, E_TRANSACTION);
		// GetAttributeValue(string) outside of transaction (Expect E_TRANSACTION)
		result = storage->GetAttributeValue(attrID, stringValue);
		EXPECT_EQ(result, E_TRANSACTION);
		// GetAttributeValue(binary) outside of transaction (Expect E_TRANSACTION)
		result = storage->GetAttributeValue(attrID, binaryValue);
		EXPECT_EQ(result, E_TRANSACTION);
		// GetAttributeValue(collection) outside of transaction (Expect E_TRANSACTION)
		result = storage->GetAttributeValue(attrID, collectionValue);
		EXPECT_EQ(result, E_TRANSACTION);
		// GetAttributeValue(pointer) outside of transaction (Expect E_TRANSACTION)
		result = storage->GetAttributeValue(attrID, pointerValue);
		EXPECT_EQ(result, E_TRANSACTION);

		// SetAttributeValue(long) outside of transaction (Expect E_TRANSACTION)
		result = storage->SetAttributeValue(attrID, longValue);
		EXPECT_EQ(result, E_TRANSACTION);
		// SetAttributeValue(real) outside of transaction (Expect E_TRANSACTION)
		result = storage->SetAttributeValue(attrID, realValue);
		EXPECT_EQ(result, E_TRANSACTION);
		// SetAttributeValue(string) outside of transaction (Expect E_TRANSACTION)
		result = storage->SetAttributeValue(attrID, stringValue);
		EXPECT_EQ(result, E_TRANSACTION);
		// SetAttributeValue(binary) outside of transaction (Expect E_TRANSACTION)
		result = storage->SetAttributeValue(attrID, binaryValue);
		EXPECT_EQ(result, E_TRANSACTION);
		// SetAttributeValue(pointer) outside of transaction (Expect E_TRANSACTION)
		result = storage->SetAttributeValue(attrID, pointerValue);
		EXPECT_EQ(result, E_TRANSACTION);
		
		result = storage->BeginTransaction();
		EXPECT_EQ(result, S_OK);

		// GetAttributeValue(long) without open object (Expect E_INVALID_USAGE)
		result = storage->GetAttributeValue(attrID, longValue);
		EXPECT_EQ(result, E_INVALID_USAGE);
		// GetAttributeValue(real) without open object (Expect E_INVALID_USAGE)
		result = storage->GetAttributeValue(attrID, realValue);
		EXPECT_EQ(result, E_INVALID_USAGE);
		// GetAttributeValue(string) without open object (Expect E_INVALID_USAGE)
		result = storage->GetAttributeValue(attrID, stringValue);
		EXPECT_EQ(result, E_INVALID_USAGE);
		// GetAttributeValue(binary) without open object (Expect E_INVALID_USAGE)
		result = storage->GetAttributeValue(attrID, binaryValue);
		EXPECT_EQ(result, E_INVALID_USAGE);
		// GetAttributeValue(collection) without open object (Expect E_INVALID_USAGE)
		result = storage->GetAttributeValue(attrID, collectionValue);
		EXPECT_EQ(result, E_INVALID_USAGE);
		// GetAttributeValue(pointer) without open object (Expect E_INVALID_USAGE)
		result = storage->GetAttributeValue(attrID, pointerValue);
		EXPECT_EQ(result, E_INVALID_USAGE);

		// SetAttributeValue(long) without open object (Expect E_INVALID_USAGE)
		result = storage->SetAttributeValue(attrID, longValue);
		EXPECT_EQ(result, E_INVALID_USAGE);
		// SetAttributeValue(real) without open object (Expect E_INVALID_USAGE)
		result = storage->SetAttributeValue(attrID, realValue);
		EXPECT_EQ(result, E_INVALID_USAGE);
		// SetAttributeValue(string) without open object (Expect E_INVALID_USAGE)
		result = storage->SetAttributeValue(attrID, stringValue);
		EXPECT_EQ(result, E_INVALID_USAGE);
		// SetAttributeValue(binary) without open object (Expect E_INVALID_USAGE)
		result = storage->SetAttributeValue(attrID, binaryValue);
		EXPECT_EQ(result, E_INVALID_USAGE);
		// SetAttributeValue(pointer) without open object (Expect E_INVALID_USAGE)
		result = storage->SetAttributeValue(attrID, pointerValue);
		EXPECT_EQ(result, E_INVALID_USAGE);

		// Get and Set AttributeValue within transaction and with open object (Expect S_OK)
		result = storage->OpenObject(idPair);
		EXPECT_EQ(result, S_OK);
		// Try changing an object's attribute value
		if (attrType == ValueType::Long())
		{
			result = storage->GetAttributeValue(attrID, longValue);
			EXPECT_EQ(result, S_OK);
			EXPECT_NE(longValue, longValueAlt);
			result = storage->SetAttributeValue(attrID, longValueAlt);
			EXPECT_EQ(result, S_OK);
			result = storage->GetAttributeValue(attrID, longValue);
			EXPECT_EQ(result, S_OK);
			EXPECT_EQ(longValue, longValueAlt);
		}
		else if (attrType == ValueType::Real())
		{
			result = storage->GetAttributeValue(attrID, realValue);
			EXPECT_EQ(result, S_OK);
			EXPECT_NE(realValue, realValueAlt);
			result = storage->SetAttributeValue(attrID, realValueAlt);
			EXPECT_EQ(result, S_OK);
			result = storage->GetAttributeValue(attrID, realValue);
			EXPECT_EQ(result, S_OK);
			EXPECT_EQ(realValue, longValueAlt);
		}
		else if (attrType == ValueType::String())
		{
			result = storage->GetAttributeValue(attrID, stringValue);
			EXPECT_EQ(result, S_OK);
			EXPECT_NE(stringValue, stringValueAlt);
			result = storage->SetAttributeValue(attrID, stringValueAlt);
			EXPECT_EQ(result, S_OK);
			result = storage->GetAttributeValue(attrID, stringValue);
			EXPECT_EQ(result, S_OK);
			EXPECT_EQ(stringValue, stringValueAlt);
		}
		else if (attrType == ValueType::Binary())
		{
//			result = storage->GetAttributeValue(attrID, realValue);
//			EXPECT_EQ(result, S_OK);
//			EXPECT_NE(realValue, realValueAlt);
//			result = storage->SetAttributeValue(attrID, realValueAlt);
//			EXPECT_EQ(result, S_OK);
//			result = storage->GetAttributeValue(attrID, realValue);
//			EXPECT_EQ(result, S_OK);
//			EXPECT_EQ(realValue, longValueAlt);
		}

		// Close the object (though not necessary since abort implies close object)
		result = storage->CloseObject();
		EXPECT_EQ(result, S_OK);
		// Abort the transaction and make sure abort reverts the attribute value
		result = storage->AbortTransaction();
		EXPECT_EQ(result, S_OK);

		// Must being another transaction and open the object
		result = storage->BeginTransaction();
		EXPECT_EQ(result, S_OK);
		result = storage->OpenObject(idPair);
		EXPECT_EQ(result, S_OK);
		// Check to make sure value has been reverted (Expect S_OK)
		if (attrType == ValueType::Long())
		{
			result = storage->GetAttributeValue(attrID, longValue);
			EXPECT_EQ(result, S_OK);
			EXPECT_NE(longValue, longValueAlt);
		}
		else if (attrType == ValueType::Real())
		{
			result = storage->GetAttributeValue(attrID, realValue);
			EXPECT_EQ(result, S_OK);
			EXPECT_NE(realValue, realValueAlt);
		}
		else if (attrType == ValueType::String())
		{
			result = storage->GetAttributeValue(attrID, stringValue);
			EXPECT_EQ(result, S_OK);
			EXPECT_NE(stringValue, stringValueAlt);
		}
		else if (attrType == ValueType::Binary())
		{
//			result = storage->GetAttributeValue(attrID, binaryValue);
//			EXPECT_EQ(result, S_OK);
//			EXPECT_NE(binaryValue, binaryValueAlt);
		}

		// Abort the transaction (though nothing has actually changed)
		result = storage->AbortTransaction();
		EXPECT_EQ(result, S_OK);		
		// Move on to the next attribute in this object
		++metaAttributeIter;
	}
}

/*
TEST_P(ICoreStorageParamTest,StringAttributeChangeWithCommit)
{
	std::string name = "";
	std::string name2 = "lkjahsliuahsd";

	// Try beginning a transaction, making an attribute value change and commiting the transaction
	result = storage->BeginTransaction();
	EXPECT_EQ(result, S_OK);
	result = storage->OpenObject(idPair);
	EXPECT_EQ(result, S_OK);
	result = storage->GetAttributeValue(ATTRID_NAME, name);
	EXPECT_EQ(result, S_OK);
	EXPECT_NE(name, name2);
	result = storage->SetAttributeValue(ATTRID_NAME, name2);
	EXPECT_EQ(result, S_OK);
	result = storage->CommitTransaction();
	EXPECT_EQ(result, S_OK);
	result = storage->BeginTransaction();
	EXPECT_EQ(result, S_OK);
	result = storage->OpenObject(idPair);
	EXPECT_EQ(result, S_OK);
	result = storage->GetAttributeValue(ATTRID_NAME, name);
	EXPECT_EQ(result, S_OK);
	EXPECT_EQ(name, name2);
	result = storage->CommitTransaction();
	EXPECT_EQ(result, S_OK);
}


TEST_P(ICoreStorageParamTest,ObjectDeleteInTransactionWithAbort)
{
	result = storage->BeginTransaction();	
	result = storage->OpenObject(idPair);
//	result = storage->DeleteObject();
	result = storage->CloseObject();
	result = storage->AbortTransaction();
}
*/

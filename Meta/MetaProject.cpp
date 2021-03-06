/*** Included Header Files ***/
#include "MetaProject.h"
#include "MetaGeneric.h"
#include "MetaFolder.h"
#include "../Core/CoreProject.h"
#include <time.h>


// --------------------------- Private Helper Functions --------------------------- //


static std::string _CurrentTime(void)
{
	time_t t;
	time(&t);
	char *p = ctime(&t);
	ASSERT( p != NULL );
	p[strlen(p)-1] = 0;
	return p;	
}


// --------------------------- Private MetaProject Methods --------------------------- //


MetaProject::MetaProject(CoreProject* &coreProject) : _coreProject(coreProject), _rootObject()
{
	ASSERT( this->_coreProject != NULL );
	// Setup the root object
	Result_t result = this->_coreProject->BeginTransaction(true);
	ASSERT( result == S_OK );
	result = this->_coreProject->RootObject(this->_rootObject);
	ASSERT( result == S_OK );
	result = this->_coreProject->CommitTransaction();
	ASSERT( result == S_OK );	
}


// --------------------------- Public MetaProject Methods --------------------------- //


MetaProject::~MetaProject()
{
	ASSERT( this->_rootObject != NULL );
	ASSERT( this->_coreProject != NULL );
	// Remove the reference to the rootObject
	this->_rootObject = NULL;
	// Now we can delete the core project
	delete this->_coreProject;
}


const Result_t MetaProject::Open(const std::string &connection, MetaProject* &project) throw()
{
	if( connection == "" ) return E_INVALID_USAGE;
	// Try to open the project
	CoreMetaProject* coreMetaProject = NULL;
	Result_t result = CreateMetaCoreMetaProject(coreMetaProject);
	ASSERT( result == S_OK );
	ASSERT( coreMetaProject != NULL );
	CoreProject *coreProject;
	result = CoreProject::Open(connection, coreMetaProject, coreProject);
	if (result != S_OK)
	{
		// Clean up and exit
		delete coreMetaProject;
		return result;
	}
	ASSERT( coreProject != NULL );
	// Create the metaProject
	MetaProject* metaProject = new MetaProject(coreProject);
	ASSERT( metaProject != NULL );
	// MetaProject is ready
	project = metaProject;
	return S_OK;
}


const Result_t MetaProject::Create(const std::string &connection, MetaProject* &project) throw()
{
	if (connection == "") return E_INVALID_USAGE;
	// Try to create a CoreProject
	CoreMetaProject* coreMetaProject = NULL;
	Result_t result = CreateMetaCoreMetaProject(coreMetaProject);
	ASSERT( result == S_OK );
	ASSERT( coreMetaProject != NULL );
	CoreProject *coreProject;
	result = CoreProject::Create(connection, coreMetaProject, coreProject);
	if (result != S_OK)
	{
		std::cout << GetErrorInfo(result);
		// Clean up and exit
		delete coreMetaProject;
		return result;
	}
	ASSERT( coreProject != NULL );
	// Create the new MetaProject
	MetaProject* metaProject = new MetaProject(coreProject);
	ASSERT( metaProject != NULL );
	// Create the root object (and some attributes)
	result = metaProject->_coreProject->BeginTransaction(false);
	ASSERT( result == S_OK );
	result = metaProject->_rootObject->SetAttributeValue(ATTRID_CDATE, _CurrentTime());
	ASSERT( result == S_OK );
	result = metaProject->_rootObject->SetAttributeValue(ATTRID_MDATE, _CurrentTime());
	ASSERT( result == S_OK );
	result = metaProject->_rootObject->SetAttributeValue(ATTRID_NAME, std::string(""));
	ASSERT( result == S_OK );
	result = metaProject->_coreProject->CommitTransaction();
	ASSERT( result == S_OK );
	// MetaProject is ready
	project = metaProject;
	return S_OK;
}


const Result_t MetaProject::Save(const std::string &filename, const bool &forceOverwrite) throw()
{
	ASSERT( this->_coreProject != NULL );
	// Ask the coreProject to save itself
	return this->_coreProject->Save(filename, forceOverwrite);
}


const Result_t MetaProject::BeginTransaction(void) throw()
{
	ASSERT( this->_coreProject != NULL );
	// BeingTransaction on underlying CoreProject
	return this->_coreProject->BeginTransaction(false);
}


const Result_t MetaProject::CommitTransaction(void) throw()
{
	ASSERT( this->_coreProject != NULL );
	// Set the modify date on the root object if the coreProject is dirty
	bool flag;
	Result_t result = this->_coreProject->IsDirty(flag);
	ASSERT( result == S_OK );
	if (flag) this->_rootObject->SetAttributeValue(ATTRID_MDATE, _CurrentTime());
	// Commit the transaction
	return this->_coreProject->CommitTransaction();
}


const Result_t MetaProject::AbortTransaction(void) throw()
{
	ASSERT( this->_coreProject != NULL );
	// AbortTransaction on underlying CoreProject
	return this->_coreProject->AbortTransaction();
}


const Result_t MetaProject::GetUuid(Uuid &uuid) const throw()
{
	Uuid value = Uuid::Null();
	// Get the value from the root object
	Result_t txResult = this->_coreProject->BeginTransaction(true);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->GetAttributeValue(ATTRID_UUID, value);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	if (result != S_OK) return result;
	// Convert GUID from bin value to GUID_t
	uuid = value;
	return S_OK;
}


const Result_t MetaProject::SetUuid(const Uuid &uuid) throw()
{
	// Set the value in the root object
	Result_t txResult = this->_coreProject->BeginTransaction(false);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->SetAttributeValue(ATTRID_UUID, uuid);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::GetName(std::string &name) const throw()
{
	// Get the value from the root object
	Result_t txResult = this->_coreProject->BeginTransaction(true);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->GetAttributeValue(ATTRID_PARNAME, name);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::SetName(const std::string &name) throw()
{
	// Set the value in the root object
	Result_t txResult = this->_coreProject->BeginTransaction(false);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->SetAttributeValue(ATTRID_PARNAME, name);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::GetDisplayedName(std::string &displayedName) const throw()
{
	// Get the value from the root object
	Result_t txResult = this->_coreProject->BeginTransaction(true);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->GetAttributeValue(ATTRID_DISPNAME, displayedName);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::SetDisplayedName(const std::string &value) throw()
{
	// Set the value in the root object
	Result_t txResult = this->_coreProject->BeginTransaction(false);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->SetAttributeValue(ATTRID_PARDISPNAME, value);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::GetVersion(std::string &version) const throw()
{
	// Get the value from the root object
	Result_t txResult = this->_coreProject->BeginTransaction(true);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->GetAttributeValue(ATTRID_VERSION, version);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::SetVersion(const std::string &value) throw()
{
	// Set the value in the root object
	Result_t txResult = this->_coreProject->BeginTransaction(false);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->SetAttributeValue(ATTRID_VERSION, value);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::GetAuthor(std::string &author) const throw()
{
	// Get the value from the root object
	Result_t txResult = this->_coreProject->BeginTransaction(true);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->GetAttributeValue(ATTRID_AUTHOR, author);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::SetAuthor(const std::string &value) throw()
{
	// Set the value in the root object
	Result_t txResult = this->_coreProject->BeginTransaction(false);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->SetAttributeValue(ATTRID_AUTHOR, value);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::GetComment(std::string &comment) const throw()
{
	// Get the value from the root object
	Result_t txResult = this->_coreProject->BeginTransaction(true);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->GetAttributeValue(ATTRID_COMMENT, comment);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::SetComment(const std::string &value) throw()
{
	// Set the value in the root object
	Result_t txResult = this->_coreProject->BeginTransaction(false);
	ASSERT( txResult == S_OK );
	Result_t result =  this->_rootObject->SetAttributeValue(ATTRID_COMMENT, value);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::GetCreatedAt(std::string &createdAt) const throw()
{
	// Get the value from the root object
	Result_t txResult = this->_coreProject->BeginTransaction(true);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->GetAttributeValue(ATTRID_CDATE, createdAt);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::SetCreatedAt(const std::string &value) throw()
{
	// Set the value in the root object
	Result_t txResult = this->_coreProject->BeginTransaction(false);
	ASSERT( txResult == S_OK );
	Result_t result =  this->_rootObject->SetAttributeValue(ATTRID_CDATE, value);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::GetModifiedAt(std::string &modifiedAt) const throw()
{
	// Get the value from the root object
	Result_t txResult = this->_coreProject->BeginTransaction(true);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->GetAttributeValue(ATTRID_MDATE, modifiedAt);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::SetModifiedAt(const std::string &value) throw()
{
	// Set the value in the root object
	Result_t txResult = this->_coreProject->BeginTransaction(false);
	ASSERT( txResult == S_OK );
	Result_t result = this->_rootObject->SetAttributeValue(ATTRID_MDATE, value);
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


const Result_t MetaProject::RootFolder(MetaFolder* &folder) throw()
{
	// Start a happy CoreProject transaction
	Result_t result = this->_coreProject->BeginTransaction(true);
	ASSERT( result == S_OK );
	// The rootObject is the rootFolder - just need to make a copy of it
	CoreObject rootObject;
	Uuid rootUuid;
	result = this->_rootObject->GetUuid(rootUuid);
	ASSERT( result == S_OK );
	result = this->_coreProject->Object(rootUuid, rootObject);
	ASSERT( result == S_OK );
	folder = new MetaFolder(rootObject, this);
	ASSERT( folder != NULL );
	// Wrap up the transaction
	result = this->_coreProject->CommitTransaction();
	ASSERT( result == S_OK );
	return S_OK;
}


const Result_t MetaProject::FindObject(const Uuid &uuid, MetaBase* &metaBase) throw()
{
	// Does this Uuid exist in the coreProject
	Result_t txResult = this->_coreProject->BeginTransaction(true);
	ASSERT( txResult == S_OK );
	CoreObject coreObject;
	// Try to find the object from the Uuid
	Result_t result = this->_coreProject->Object(uuid, coreObject);
	// If the object was found, create a metaBase from it
	if (coreObject != NULL) metaBase = new MetaBase(coreObject, this);
	else metaBase = NULL;
	txResult = this->_coreProject->CommitTransaction();
	ASSERT( txResult == S_OK );
	return result;
}


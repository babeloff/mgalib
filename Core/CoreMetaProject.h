#ifndef __CORE_METAPROJECT_H__
#define __CORE_METAPROJECT_H__


/*** Included Header Files ***/
#include "CoreTypes.h"


/*** Namespace Declaration ***/
namespace MGA {
	

/*** Class Predefinitions ***/
class CoreMetaObject;


// --------------------------- CoreMetaProject ---------------------------


class CoreMetaProject 
{
private:
	CoreMetaProject();											//!< Deny access to default constructor
	CoreMetaProject(const CoreMetaProject &);					//!< Deny access to copy constructor
	CoreMetaProject& operator=(const CoreMetaProject &);		//!< Deny access to equals operator

protected:
	std::list<CoreMetaObject*>					_objects;		//!< metaObjects in the metaProject
	std::string									_token;			//!< Short name of the metaProject
	std::string									_name;			//!< Full name of the metaProject
	Uuid										_uuid;			//!< Unique Uuid of the metaProject
public:
	CoreMetaProject(const std::string &token, const std::string &name, const Uuid &uuid) :
		_objects(), _token(token), _name(name), _uuid(uuid) { }
	~CoreMetaProject();

	inline const Result_t GetUuid(Uuid &uuid) const throw()				{ uuid = this->_uuid; return S_OK; }
	inline const Result_t GetToken(std::string &token) const throw()	{ token = this->_token; return S_OK; }
	inline const Result_t GetName(std::string &name) const throw()		{ name = this->_name; return S_OK; }
	inline const Result_t GetObjects(std::list<CoreMetaObject*> &objectList) const throw()	{ objectList = this->_objects; return S_OK; }
	const Result_t GetObject(const MetaID_t &metaID, CoreMetaObject* &object) throw();
	const Result_t AddObject(const MetaID_t &metaID, const std::string &token, const std::string &name,
							 CoreMetaObject* &object) throw();
};


/*** End of MGA Namespace ***/
}


#endif	//__CORE_METAPROJECT_H__

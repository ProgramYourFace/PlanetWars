#include "Plugin.h"
#include "Resource.h"


Resource::Resource()
{
}


Resource::~Resource()
{
}

EXPORT void DeleteNativeResouce(Resource*& resource)
{
	SAFE_DELETE(resource);
}
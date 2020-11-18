#pragma once

#include <vulkan/vulkan.h>

namespace Rosee {

void vkAssert(VkResult res);

namespace Vk {

template <typename HandleType>
class Handle
{
	HandleType m_handle;

public:
	Handle(HandleType handle) :
		m_handle(handle)
	{
	}

	operator HandleType(void) const
	{
		return m_handle;
	}

	operator bool(void) const
	{
		return m_handle != VK_NULL_HANDLE;
	}
};

class Instance : public Handle<VkInstance>
{
public:
	Instance(VkInstance instance) :
		Handle<VkInstance>(instance)
	{
	}

private:
	void* getProcAddrImpl(const char *name) const;

public:
	template <typename ProcType>
	ProcType getProcAddr(const char *name) const
	{
		return reinterpret_cast<ProcType>(getProcAddrImpl(name));
	}
};

class DebugUtilsMessengerEXT : public Handle<VkDebugUtilsMessengerEXT>
{
public:
	DebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT messenger) :
		Handle<VkDebugUtilsMessengerEXT>(messenger)
	{
	}
};

using SurfaceKHR = Handle<VkSurfaceKHR>;
using Device = Handle<VkDevice>;

}
}
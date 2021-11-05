
#ifndef _LOCAL_AUXILLARY_HPP_
#define _LOCAL_AUXILLARY_HPP_


#include <semaphore>


class semaphore_storage_t
{
	friend class swapchain_t;

	std::binary_semaphore m_semaphore;

public:
	semaphore_storage_t() noexcept
		: m_semaphore(1)
	{}

	semaphore_storage_t(semaphore_storage_t&&) noexcept
		: m_semaphore(1)
	{}

	void acquire() noexcept
	{
		this->m_semaphore.acquire();
	}

	void release() noexcept
	{
		this->m_semaphore.release();
	}
};



class exception_with_code
{
private:
	
	std::wstring m_excp;
	int m_code;


public:
	
	exception_with_code(int code, const wchar_t* what)
		: m_code(code), m_excp(what) {}

	exception_with_code(int code, const std::wstring& what)
		: m_code(code), m_excp(what) {}

	exception_with_code(int code, std::wstring&& what)
		: m_code(code), m_excp(std::forward<std::wstring>(what)) {}


	int code() const
	{
		return this->m_code;
	}

	const wchar_t* what() const
	{
		return this->m_excp.data();
	}
};



#endif //!_LOCAL_AUXILLARY_HPP_

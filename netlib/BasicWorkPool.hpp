#ifndef BASIC_WORKPOOL_H
#define BASIC_WORKPOOL_H

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/aligned_storage.hpp>
#include "NetLibTypes.hpp"

namespace netlib
{
	inline static uint8_t GetMaxThreadConcurrency()
	{
		return boost::thread::hardware_concurrency();
	}

	class BasicWorkPool
		:private boost::noncopyable
	{
	public:
		BasicWorkPool(std::size_t core=1);

		boost::asio::io_service&        GetIoService();
		void                            StartWorkPool();
		void                            StopWorkPool();
		~BasicWorkPool() { StopWorkPool();}

	private:
		typedef boost::shared_ptr<boost::asio::io_service>       IoServicePtr;
		typedef boost::shared_ptr<boost::asio::io_service::work> WorkPtr;

		std::size_t              m_CoreCount;
		// Services in the pool
		std::vector<IoServicePtr>        m_Services;
		// The work that keeps the io_services running. 
		std::vector<WorkPtr>        m_Works;
		// The next io_service . 
		std::size_t              m_NextIoService;
		// Work threads
		boost::thread_group      m_Threads;
	};



	// Class to manage the memory to be used for handler-based custom allocation.
	// It contains a single block of memory which may be returned for allocation
	// requests. If the memory is in use when an allocation request is made, the
	// allocator delegates allocation to the global heap.

	// Thread Safety: None

	class HandlerAllocator : private boost::noncopyable
	{
	public:
		HandlerAllocator() : m_bInUse(false) {}

		void* Allocate(std::size_t size) {
			if (!m_bInUse && size < m_Storage.size){
				m_bInUse = true;
				return m_Storage.address();
			}
			else {
				return ::operator new(size);
			}
		}  

		void Deallocate(void* pAddress) {
			if (pAddress == m_Storage.address()) {
				m_bInUse = false;
			}
			else {
				::operator delete(pAddress);
			}
		}

	private:  
		// Storage space used for handler-based custom memory allocation.
		boost::aligned_storage<1024> m_Storage;  

		// Whether the handler-based custom allocation storage has been used.
		bool                        m_bInUse;
	};

	// Wrapper class template for handler objects to allow handler memory
	// allocation to be customised. Calls to operator() are forwarded to the
	// encapsulated handler.
	template <typename Handler>
	class CustomAllocateHandler
	{
	public:
		CustomAllocateHandler(HandlerAllocator& allocator, Handler handler) :
		  m_Allocator(allocator),
			  m_Handler(handler)  {}

		  void operator()() {
			  m_Handler();
		  }

		  template <typename Arg1>
		  void operator()(Arg1 arg1) {
			  m_Handler(arg1);
		  }  

		  template <typename Arg1,
			  typename Arg2>
			  void operator()(Arg1 arg1, Arg2 arg2) {
				  m_Handler(arg1, arg2);
		  }

		  template <typename Arg1,
			  typename Arg2,
			  typename Arg3>
			  void operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3) {
				  m_Handler(arg1, arg2, arg3);
		  }

		  template <typename Arg1,
			  typename Arg2,
			  typename Arg3,
			  typename Arg4>
			  void operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
				  m_Handler(arg1, arg2, arg3, arg4);
		  }

		  template <typename Arg1,
			  typename Arg2,
			  typename Arg3,
			  typename Arg4,
			  typename Arg5>
			  void operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) {
				  m_Handler(arg1, arg2, arg3, arg4, arg5);
		  }

		  friend void* asio_handler_allocate(std::size_t size,  CustomAllocateHandler<Handler>* this_handler) {
			  return this_handler->m_Allocator.Allocate(size);
		  }

		  friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, CustomAllocateHandler<Handler>* this_handler)  {
			  this_handler->m_Allocator.Deallocate(pointer);
		  }

	private:  
		HandlerAllocator&            m_Allocator;
		Handler                        m_Handler;
	};

	// Helper function to wrap a handler object to add custom allocation.
	template <typename Handler>
	inline CustomAllocateHandler<Handler>
		MakeCustomizeHandler(HandlerAllocator& allocator, Handler handler) {
			return CustomAllocateHandler<Handler>(allocator, handler);
	}
}

#include "impl/BasicWorkPool.ipp"

#endif
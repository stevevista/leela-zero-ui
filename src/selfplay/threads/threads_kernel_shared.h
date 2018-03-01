// Copyright (C) 2003  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_THREADS_KERNEl_SHARED_
#define DLIB_THREADS_KERNEl_SHARED_

// this file should be included at the bottom of one of the thread kernel headers for a 
// specific platform.
//#include "../threads.h"
#include "auto_mutex_extension.h"
#include "../memory_manager_kernel_2.h"

#include "binary_search_tree_kernel_1.h"
#include "binary_search_tree_kernel_2.h"

#include "set_kernel_1.h"
#include "set_kernel_c.h"

#include "../matrix/algs.h"
#include <functional>

namespace dlib
{
    template <
        typename domain,
        typename range,
        typename mem_manager = default_memory_manager,
        typename compare = std::less<domain>
        >
    class binary_search_tree
    {
        binary_search_tree() {}

    public:
        
        //----------- kernels ---------------

        // kernel_1a        
        typedef     binary_search_tree_kernel_1<domain,range,mem_manager,compare>    
                    kernel_1a;

        // kernel_2a        
        typedef     binary_search_tree_kernel_2<domain,range,mem_manager,compare>    
                    kernel_2a;

    };

    template <
        typename T,
        typename mem_manager = default_memory_manager,
        typename compare = std::less<T>
        >
    class set
    {
        set() {}

        typedef typename binary_search_tree<T,char,mem_manager,compare>::kernel_2a
                binary_search_tree_2;

    public:
        
        //----------- kernels ---------------
        // kernel_1b
        typedef     set_kernel_1<T,binary_search_tree_2,mem_manager>
                    kernel_1b;
        typedef     set_kernel_c<kernel_1b>
                    kernel_1b_c;
    };


    template <
        typename T
        >
    class memory_manager
    {
        memory_manager() {}

    public:
      
        // kernel_2        
        typedef      memory_manager_kernel_2<T,10>
                     kernel_2a;
        typedef      memory_manager_kernel_2<T,100>
                     kernel_2b;

    };
}


extern "C"
{
// =========================>>> WHY YOU ARE GETTING AN ERROR HERE <<<=========================
// The point of this block of code is to cause a link time error that will prevent a user
// from compiling part of their application with DLIB_ASSERT enabled and part with it
// disabled since doing that would be a violation of C++'s one definition rule.  So if you
// are getting an error here then you are either not enabling DLIB_ASSERT consistently
// (e.g. by compiling part of your program in a debug mode and part in a release mode) or
// you have simply forgotten to compile dlib/all/source.cpp into your application.
// =========================>>> WHY YOU ARE GETTING AN ERROR HERE <<<=========================
#ifdef ENABLE_ASSERTS
    extern int USER_ERROR__missing_dlib_all_source_cpp_file__OR__inconsistent_use_of_DEBUG_or_ENABLE_ASSERTS_preprocessor_directives;
    inline int dlib_check_consistent_assert_usage() { USER_ERROR__missing_dlib_all_source_cpp_file__OR__inconsistent_use_of_DEBUG_or_ENABLE_ASSERTS_preprocessor_directives = 0; return 0; }
#else
    extern int USER_ERROR__missing_dlib_all_source_cpp_file__OR__inconsistent_use_of_DEBUG_or_ENABLE_ASSERTS_preprocessor_directives_;
    inline int dlib_check_consistent_assert_usage() { USER_ERROR__missing_dlib_all_source_cpp_file__OR__inconsistent_use_of_DEBUG_or_ENABLE_ASSERTS_preprocessor_directives_ = 0; return 0; }
#endif
    const int DLIB_NO_WARN_UNUSED dlib_check_assert_helper_variable = dlib_check_consistent_assert_usage();
}














namespace dlib
{


// ----------------------------------------------------------------------------------------

    namespace threads_kernel_shared
    {
        void thread_starter (
            void*
        );

        class threader
        {
            /*!
                INITIAL VALUE
                    - pool_count == 0 and
                    - data_ready is associated with the mutex data_mutex 
                    - data_empty is associated with the mutex data_mutex
                    - destructed is associated with the mutex data_mutex
                    - destruct == false
                    - total_count == 0
                    - function_pointer == 0
                    - do_not_ever_destruct == false

                CONVENTION
                    - data_ready is associated with the mutex data_mutex 
                    - data_empty is associated with the mutex data_mutex 
                    - data_ready == a signaler used signal when there is new data waiting 
                      to start a thread with.
                    - data_empty == a signaler used to signal when the data is now empty 
                    - pool_count == the number of suspended threads in the thread pool 
                    - total_count == the number of threads that are executing anywhere.  i.e.
                      pool_count + the ones that are currently running some user function.
                    - if (function_pointer != 0) then
                        - parameter == a void pointer pointing to the parameter which 
                          should be used to start the next thread 
                        - function_pointer == a pointer to the next function to make a 
                          new thread with

                    - if (the destructor is running) then
                        - destruct == true
                    - else
                        - destruct == false

                    - thread_ids is locked by the data_mutex
                    - thread_ids == a set that contains the thread id for each thread spawned by this
                      object.
            !*/


        public:
            threader (
            );
           
            ~threader (
            );

            void destruct_if_ready (
            );
            /*!
                ensures
                    - if (there are no threads currently running and we haven't set do_not_ever_destruct) then
                        - calls delete this
                    - else
                        - does nothing
            !*/

            bool create_new_thread (
                void (*funct)(void*),
                void* param
            );

            bool is_dlib_thread (
                thread_id_type id
            );

        private:

            friend void thread_starter (
                void*
            );

            // private data
            set<thread_id_type,memory_manager<char>::kernel_2b>::kernel_1b_c thread_ids;
            unsigned long total_count;
            void* parameter;
            void (*function_pointer)(void*);
            unsigned long pool_count;
            mutex data_mutex;           // mutex to protect the above data
            signaler data_ready;        // signaler to signal when there is new data
            signaler data_empty;        // signaler to signal when the data is empty
            bool destruct;
            signaler destructed;        // signaler to signal when a thread has ended 
            bool do_not_ever_destruct;

            // restricted functions
            threader(threader&);        // copy constructor
            threader& operator=(threader&);    // assignement opertor

        };

    // ------------------------------------------------------------------------------------

        threader& thread_pool (
        ); 
        /*!
            ensures
                - returns a reference to the global threader object
        !*/

    // ------------------------------------------------------------------------------------

        extern bool thread_pool_has_been_destroyed;
    }

    bool is_dlib_thread (
        thread_id_type id 
    );

    bool is_dlib_thread (
    );

// ----------------------------------------------------------------------------------------

    inline bool create_new_thread (
        void (*funct)(void*),
        void* param
    )
    {
        try
        {
            // now make this thread
            return threads_kernel_shared::thread_pool().create_new_thread(funct,param);
        }
        catch (std::bad_alloc&)
        {
            return false;
        }
    }

// ----------------------------------------------------------------------------------------

}

#ifdef NO_MAKEFILE
#include "threads_kernel_shared.cpp"
#endif

#endif // DLIB_THREADS_KERNEl_SHARED_


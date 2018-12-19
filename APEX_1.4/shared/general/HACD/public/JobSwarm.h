#ifndef JOB_SWARM_H

#define JOB_SWARM_H

/*!
**
** Copyright (c) 20011 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "PlatformConfigHACD.h"

namespace JOB_SWARM
{

// Any 'Job' you want to execute should inherit this class and implement the three virtual methods.
// 'job_process' defines the 'work' you want to do and runs in a seperate thread.  It must be thread safe code.
// job_onFinish is called from the main application thread and when the job has completed so you can do something with the results.
// job_onCancel is called from the main application thread when this job was cancelled so any cleanup necessary can be performed.
class JobSwarmInterface
{
public:
  virtual void job_process(void *userData,hacd::HaI32 userId) = 0;   // RUNS IN ANOTHER THREAD!! MUST BE THREAD SAFE!
  virtual void job_onFinish(void *userData,hacd::HaI32 userId) = 0;  // runs in primary thread of the context
  virtual void job_onCancel(void *userData,hacd::HaI32 userId) = 0;  // runs in primary thread of the context
};


class SwarmJob;

// This is a single instance of a JobSwarm system.  You can have multiple JobSwarmContexts in a single application.
class JobSwarmContext
{
public:

  virtual SwarmJob *   createSwarmJob(JobSwarmInterface *iface,void *userData,hacd::HaI32 userId) = 0; // creates a job to be processed and returns a handle.
  virtual void         cancel(SwarmJob *job) = 0; // cancels the job, use cannot delete the memory until he receives the onCancel event!

  virtual bool processSwarmJobs(void) = 0; // This is a pump loop run in the main thread to handle the disposition of finished and/or cancelled jobs.  Returns true if there are still outstanding jobs not yet full procesed.
  virtual void         setUseThreads(bool state) = 0; // Whether or not to run in hardware threads.  This is for debugging only, threading is always true by default.
};

JobSwarmContext * createJobSwarmContext(hacd::HaU32 maxThreadCount=4); // create a JobSwarmContext with the give number of physical threads
bool              releaseJobSwarmContext(JobSwarmContext *c); // release a JobSwarmContet


}; // end of namespace


#endif

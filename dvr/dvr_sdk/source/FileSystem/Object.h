#ifndef __OBJECT_H__
#define __OBJECT_H__
#include <string.h>

#define	FSDel(_obj_) 		do{ if(_obj_) { delete (_obj_); (_obj_) = NULL;} } while(0)
#define	FSFree(_p_)			do{ if(_p_){ free(_p_); (_p_)=NULL;}}while(0)
#define	BIT(x)				(unsigned int)(1<<(x))
#define	BIT_ISSET(v, b)		((v)&BIT(b))
#define BIT_SET(v, b)		(v) = (v) | BIT(b)
#define	BIT_CLR(v, b)		(v) = (v) & (~BIT(b) )
#define	BIT_ISCLR(v, b) 	(!BIT_ISSET(v, b))
class CObject {
public:
	CObject(){};
	virtual ~CObject(){};
};

/////////////////////////////////////////////////
///////////////// single mode
#define PATTERN_SINGLETON_DECLARE(classname)	\
static classname * instance();

#define PATTERN_SINGLETON_IMPLEMENT(classname)	\
classname * classname::instance()		\
{												\
	static classname * _instance = NULL;		\
	if( NULL == _instance)						\
	{											\
		_instance = new classname;				\
	}											\
	return _instance;							\
}												
/////////////////////////////////////////////////
#endif// __OBJECT_H__

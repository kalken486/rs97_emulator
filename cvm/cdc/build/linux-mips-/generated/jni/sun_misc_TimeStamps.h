/* DO NOT EDIT THIS FILE - it is machine generated */
#include "javavm/export/jni.h"
/* Header for class sun/misc/TimeStamps */

#ifndef _CVM_JNI_sun_misc_TimeStamps
#define _CVM_JNI_sun_misc_TimeStamps
#ifdef __cplusplus
extern "C"{
#endif
/*
 * Class:	sun/misc/TimeStamps
 * Method:	enable
 * Signature:	()V
 */
JNIEXPORT void JNICALL Java_sun_misc_TimeStamps_enable
  (JNIEnv *, jclass);

/*
 * Class:	sun/misc/TimeStamps
 * Method:	isEnabled
 * Signature:	()Z
 */
JNIEXPORT jboolean JNICALL Java_sun_misc_TimeStamps_isEnabled
  (JNIEnv *, jclass);

/*
 * Class:	sun/misc/TimeStamps
 * Method:	recordTimeStamps
 * Signature:	(Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_sun_misc_TimeStamps_recordTimeStamps__Ljava_lang_String_2
  (JNIEnv *, jclass, jstring);

/*
 * Class:	sun/misc/TimeStamps
 * Method:	recordTimeStamps
 * Signature:	(I)Z
 */
JNIEXPORT jboolean JNICALL Java_sun_misc_TimeStamps_recordTimeStamps__I
  (JNIEnv *, jclass, jint);

/*
 * Class:	sun/misc/TimeStamps
 * Method:	printTimeStamps
 * Signature:	()Z
 */
JNIEXPORT jboolean JNICALL Java_sun_misc_TimeStamps_printTimeStamps
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif

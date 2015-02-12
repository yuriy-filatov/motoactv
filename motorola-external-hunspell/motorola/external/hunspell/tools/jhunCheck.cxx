/*
 * Copyright (C) 2010/2011 Motorola Inc.
 * All Rights Reserved.
 * Motorola Confidential Restricted.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <jni.h>
#include <errno.h>

#include "hunspell.hxx"
#include "com_mot_android_HunChecker.h"

#include<utils/Log.h>
#define LOG_TAG "$$$HUNSPELL$$$"

using namespace std;
#define MAXMACH 10

struct machines {
    Hunspell *mptr;
} hmachines[MAXMACH];

static int add_to_machines(Hunspell *ptr)
{
    register int i;
    for(i = 0; i < MAXMACH; i++) {
        if(hmachines[i].mptr == NULL) {
            hmachines[i].mptr = ptr;
            return i+1;
        }
    }

    return 0;
}

static void delete_from_machines(int mnr)
{
    int index = mnr - 1;
    if(index < MAXMACH && hmachines[index].mptr != NULL) {
        delete(hmachines[index].mptr);
        hmachines[index].mptr = NULL;
    }
}

Hunspell *get_hunspell_ptr(int mnr)
{
    if((mnr >= 1) && (mnr <= MAXMACH))
        return hmachines[mnr-1].mptr;
    else
        return NULL;
}

JNIEXPORT jint JNICALL Java_com_mot_android_HunChecker_openDict(JNIEnv *env, jclass c, jstring jname)
{
    char *aff, *dic;
    Hunspell *pMS = NULL;
    int retval;
    const char *name = env->GetStringUTFChars(jname, 0);
    aff = (char *)malloc(strlen(name)+5);
    dic = (char *)malloc(strlen(name)+5);
    strcpy(aff, name);
    strcat(aff, ".aff");
    strcpy(dic, name);
    strcat(dic, ".dic");
    if(aff && dic){
        pMS= new Hunspell(aff,dic);
    }

    env->ReleaseStringUTFChars(jname, name);

    if(aff)
        free(aff);

    if(dic)
        free(dic);

    if(pMS)
        retval = add_to_machines(pMS);
    else 
        retval = 0;

    return retval;
}

JNIEXPORT jint JNICALL Java_com_mot_android_HunChecker_addDict(JNIEnv *env, jclass c, jint hp, jstring jname)
{
    Hunspell *pMS;
    char *dic;
    int ret = 0;
    pMS = get_hunspell_ptr(hp);
    if(pMS) {
        const char *name = env->GetStringUTFChars(jname, 0);
        dic = (char *)malloc(strlen(name)+5);
        if(dic) {
            strcpy(dic, name);
            strcat(dic, ".dic");
            ret = pMS->add_dic(dic);

            free(dic);
        }
        env->ReleaseStringUTFChars(jname, name);
    }

    return ret;
}

JNIEXPORT jint JNICALL Java_com_mot_android_HunChecker_checkWord(JNIEnv *env, jclass c, jint hp, jbyteArray data)
{
    Hunspell *pMS;
    pMS = get_hunspell_ptr(hp);
    if(pMS) {
        jbyte *body = env->GetByteArrayElements(data, 0);
        jsize size = env->GetArrayLength(data);

        char* wd = (char *)malloc(size + 1);
        if(wd) {
            memcpy(wd, body, size);
            wd[size] = '\0';
            //LOGD("checking word: %s, length: %d",wd, size);
            int dp = pMS->spell(wd);

            free(wd);

            if(dp) {
                env->ReleaseByteArrayElements(data, body, 0);
                return 1;
            }
        }

        env->ReleaseByteArrayElements(data, body, 0);
    }

    return 0;
}

#define MAXSUG 256
JNIEXPORT jbyteArray JNICALL Java_com_mot_android_HunChecker_checkSug(JNIEnv *env, jclass c, jint hp, jbyteArray data)
{
    Hunspell *pMS;
    pMS = get_hunspell_ptr(hp);
    if(pMS) {
        jbyte *body = env->GetByteArrayElements(data, 0);
        jsize size = env->GetArrayLength(data);

        char* wd = (char *)malloc(size + 1);
        if(wd) {
            memcpy(wd, body, size);
            wd[size] = '\0';

            //LOGD("getting suggestions of word: %s",wd);

            int dp = pMS->spell(wd);
            if(dp) {
                free(wd);
                env->ReleaseByteArrayElements(data, body, 0);
                return NULL;
            } else{
                char ** wlst;
                int suglen = 0;
                int ns = pMS->suggest(&wlst,wd);
                char sug[MAXSUG];
                *sug = 0;
                for (int i=0; i < ns; i++) {
                    if(suglen + strlen(wlst[i]) + 1 < MAXSUG) {
                        //LOGD("suggestion number %d for %s: %s, length: %d", i, wd, wlst[i], strlen(wlst[i]));
                        strcat(sug,wlst[i]);
                        strcat(sug, "$");
                        suglen += strlen(wlst[i]) + 1;
                    }
                }

                free(wd);
                env->ReleaseByteArrayElements(data, body, 0);
                pMS->free_list(&wlst, ns);
                jbyteArray jbarray = env->NewByteArray(suglen);
                env->SetByteArrayRegion(jbarray, 0, suglen, (const jbyte *)sug);
                return jbarray;
            }
        }
        env->ReleaseByteArrayElements(data, body, 0);
    }

    return NULL;
}

JNIEXPORT void JNICALL Java_com_mot_android_HunChecker_addWord(JNIEnv *env, jclass c, jint hp, jbyteArray data)
{
    Hunspell *pMS = get_hunspell_ptr(hp);
    if(pMS) {
        jbyte *body = env->GetByteArrayElements(data, 0);
        jsize size = env->GetArrayLength(data);

        char* wd = (char *)malloc(size + 1);
        if(wd) {
            memcpy(wd, body, size);
            wd[size] = '\0';
            pMS->add(wd);

            free(wd);
        }
        env->ReleaseByteArrayElements(data, body, 0);
    }
    return;
}

JNIEXPORT void JNICALL Java_com_mot_android_HunChecker_removeWord(JNIEnv *env, jclass c, jint hp, jbyteArray data)
{
    Hunspell *pMS = get_hunspell_ptr(hp);
    if(pMS) {
        jbyte *body = env->GetByteArrayElements(data, 0);
        jsize size = env->GetArrayLength(data);

        char* wd = (char *)malloc(size + 1);
        if(wd) {
            memcpy(wd, body, size);
            wd[size] = '\0';
            pMS->remove(wd);

            free(wd);
        }
        env->ReleaseByteArrayElements(data, body, 0);
    }
    return;
}

JNIEXPORT jstring JNICALL Java_com_mot_android_HunChecker_getDictEncoding(JNIEnv *env, jclass c, jint hp)
{
    Hunspell *pMS = get_hunspell_ptr(hp);
    jstring ret = 0;
    if(pMS) {
        char *dic_encoding = pMS->get_dic_encoding();
        ret = env->NewStringUTF(dic_encoding);
    } else {
        ret = env->NewStringUTF("unknown");
    }

    return ret;
}


JNIEXPORT void JNICALL Java_com_mot_android_HunChecker_closeDict(JNIEnv *env, jclass c, jint hp)
{
    delete_from_machines(hp);
    return;
}

/*--------------------------- FUNCTION DEFINITION ----------------------------*/
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) 
{ 
    JNIEnv    *env;
    jclass    k;
    jint      r;
    JNINativeMethod meth[8];

    r = vm->GetEnv((void **) &env, JNI_VERSION_1_4);
    k = env->FindClass("com/motorola/spellingcheckservice/SpellingCheckService");

    if(k==NULL)
    {
        return -1;
    } 

    meth[0].name      = "nativeOpenDict";
    meth[0].signature = "(Ljava/lang/String;)I";
    meth[0].fnPtr     = (void*)Java_com_mot_android_HunChecker_openDict;

    meth[1].name      = "nativeAddDict";
    meth[1].signature = "(ILjava/lang/String;)I";
    meth[1].fnPtr     = (void*)Java_com_mot_android_HunChecker_addDict;

    meth[2].name      = "nativeCheckWord";
    meth[2].signature = "(I[B)I";
    meth[2].fnPtr     = (void*)Java_com_mot_android_HunChecker_checkWord;

    meth[3].name      = "nativeCheckSug";
    meth[3].signature = "(I[B)[B";
    meth[3].fnPtr     = (void*)Java_com_mot_android_HunChecker_checkSug;

    meth[4].name      = "nativeAddWord";
    meth[4].signature = "(I[B)V";
    meth[4].fnPtr     = (void*)Java_com_mot_android_HunChecker_addWord;

    meth[5].name      = "nativeRemoveWord";
    meth[5].signature = "(I[B)V";
    meth[5].fnPtr     = (void*)Java_com_mot_android_HunChecker_removeWord;
    
    meth[6].name      = "nativeGetDictEncoding";
    meth[6].signature = "(I)Ljava/lang/String;";
    meth[6].fnPtr     = (void*)Java_com_mot_android_HunChecker_getDictEncoding;

    meth[7].name      = "nativeCloseDict";
    meth[7].signature = "(I)V";
    meth[7].fnPtr     = (void*)Java_com_mot_android_HunChecker_closeDict;

    r = env->RegisterNatives (k, meth, 8);

    return JNI_VERSION_1_4;
} 

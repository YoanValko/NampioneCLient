#pragma once

#ifndef JNI_HELPER_H
#define JNI_HELPER_H

#include <jni.h>
#include "nemotron_common.h"

namespace Nemotron {

    // JNI Helper functions
    class JNIHelper {
    public:
        static JNIEnv* GetJNIEnv();
        static void CacheJNIEnvironment(JNIEnv* env);
        static bool InitializeJNICache(JNIEnv* env);
        
        // Class finding
        static jclass FindClass(JNIEnv* env, const char* name);
        static jclass GetClass(JNIEnv* env, const char* name);
        
        // Field/Method access
        static jfieldID GetFieldID(JNIEnv* env, jclass clazz, const char* name, const char* sig);
        static jmethodID GetMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig);
        static jmethodID GetStaticMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig);
        
        // Object manipulation
        template<typename T>
        static T GetField(JNIEnv* env, jobject obj, jfieldID field) {
            return static_cast<T>(env->GetObjectField(obj, field));
        }
        
        template<typename T>
        static T GetStaticField(JNIEnv* env, jclass clazz, jfieldID field) {
            return static_cast<T>(env->GetStaticObjectField(clazz, field));
        }
        
        template<typename T>
        static void SetField(JNIEnv* env, jobject obj, jfieldID field, T value) {
            env->SetObjectField(obj, field, value);
        }
        
        // Method calling
        template<typename T, typename... Args>
        static T CallMethod(JNIEnv* env, jobject obj, jmethodID method, Args... args) {
            return static_cast<T>(env->CallObjectMethod(obj, method, args...));
        }
        
        template<typename T, typename... Args>
        static T CallStaticMethod(JNIEnv* env, jclass clazz, jmethodID method, Args... args) {
            return static_cast<T>(env->CallStaticObjectMethod(clazz, method, args...));
        }
        
        // Primitive field access
        static jint GetIntField(JNIEnv* env, jobject obj, jfieldID field) {
            return env->GetIntField(obj, field);
        }
        
        static jfloat GetFloatField(JNIEnv* env, jobject obj, jfieldID field) {
            return env->GetFloatField(obj, field);
        }
        
        static jdouble GetDoubleField(JNIEnv* env, jobject obj, jfieldID field) {
            return env->GetDoubleField(obj, field);
        }
        
        static jboolean GetBooleanField(JNIEnv* env, jobject obj, jfieldID field) {
            return env->GetBooleanField(obj, field);
        }
        
        static void SetIntField(JNIEnv* env, jobject obj, jfieldID field, jint value) {
            env->SetIntField(obj, field, value);
        }
        
        static void SetFloatField(JNIEnv* env, jobject obj, jfieldID field, jfloat value) {
            env->SetFloatField(obj, field, value);
        }
        
        static void SetBooleanField(JNIEnv* env, jobject obj, jfieldID field, jboolean value) {
            env->SetBooleanField(obj, field, value);
        }

        // Array operations
        static jobjectArray NewObjectArray(JNIEnv* env, jsize length, jclass clazz, jobject initialElement) {
            return env->NewObjectArray(length, clazz, initialElement);
        }
        
        static jsize GetArrayLength(JNIEnv* env, jarray array) {
            return env->GetArrayLength(array);
        }
        
        static jobject GetObjectArrayElement(JNIEnv* env, jobjectArray array, jsize index) {
            return env->GetObjectArrayElement(array, index);
        }
        
        // String operations
        static jstring NewStringUTF(JNIEnv* env, const char* str) {
            return env->NewStringUTF(str);
        }
        
        static const char* GetStringUTFChars(JNIEnv* env, jstring str, jboolean* isCopy) {
            return env->GetStringUTFChars(str, isCopy);
        }
        
        static void ReleaseStringUTFChars(JNIEnv* env, jstring str, const char* chars) {
            env->ReleaseStringUTFChars(str, chars);
        }
        
        // Exception handling
        static void CheckAndClearException(JNIEnv* env);
        static void ThrowException(JNIEnv* env, const char* className, const char* message);
        
        // Object creation
        static jobject NewObject(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
        static jobject NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID, jvalue* args);
    };

    // Math helpers for aimbot
    struct Vec3 {
        float x, y, z;
        
        Vec3() : x(0), y(0), z(0) {}
        Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
        
        Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
        Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
        Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
        Vec3 operator/(float scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }
        
        float Length() const { return sqrtf(x*x + y*y + z*z); }
        float Length2D() const { return sqrtf(x*x + z*z); }
        
        Vec3 Normalized() const {
            float len = Length();
            if (len == 0) return Vec3(0, 0, 0);
            return *this / len;
        }
        
        float DistanceTo(const Vec3& other) const {
            return (*this - other).Length();
        }
    };
    
    // Rotation helpers
    struct Rotation {
        float yaw, pitch;
        
        Rotation() : yaw(0), pitch(0) {}
        Rotation(float yaw, float pitch) : yaw(yaw), pitch(pitch) {}
    };
    
    // Calculate angle between two vectors
    Rotation CalculateAngle(const Vec3& from, const Vec3& to);
    
    // Normalize angles
    void NormalizeAngles(Rotation& rot);
    
    // Smooth rotation
    Rotation SmoothRotation(const Rotation& current, const Rotation& target, float smoothFactor);
    
    // Linear interpolation
    float Lerp(float a, float b, float t);
    Vec3 Lerp(const Vec3& a, const Vec3& b, float t);
    
    // Random utilities
    float RandomFloat(float min, float max);
    int RandomInt(int min, int max);
    Vec3 RandomVec3(float radius);

} // namespace Nemotron

#endif // JNI_HELPER_H
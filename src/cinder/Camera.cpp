/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.
 
 Portions of this code (C) Paul Houx
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "cinder/Camera.h"
#include "cinder/Sphere.h"

#include "cinder/CinderMath.h"
#include "cinder/Matrix33.h"

#include "glm/glm.hpp"
#include "glm/gtc/Quaternion.hpp"
#include "glm/gtx/Quaternion.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace cinder {

void Camera::setEyePoint( const Vec3f &aEyePoint )
{
	mEyePoint = aEyePoint;
	mModelViewCached = false;
}

void Camera::setCenterOfInterestPoint( const Vec3f &centerOfInterestPoint )
{
	mCenterOfInterest = mEyePoint.distance( centerOfInterestPoint );
	lookAt( centerOfInterestPoint );
}

void Camera::setViewDirection( const Vec3f &aViewDirection )
{
	mViewDirection = aViewDirection.normalized();
	mOrientation = glm::rotation( toGlm( mViewDirection ), glm::vec3( 0, 0, -1 ) );
	mModelViewCached = false;
}

void Camera::setOrientation( const Quatf &aOrientation )
{
	mOrientation = glm::normalize( aOrientation );
	mViewDirection = fromGlm( glm::rotate( mOrientation, glm::vec3( 0, 0, -1 ) ) );
	mModelViewCached = false;
}

void Camera::setWorldUp( const Vec3f &aWorldUp )
{
	mWorldUp = aWorldUp.normalized();
	mOrientation = Quatf( glm::toQuat( alignZAxisWithTarget( -mViewDirection, mWorldUp ) ) );
	mModelViewCached = false;
}

void Camera::lookAt( const Vec3f &target )
{
	mViewDirection = ( target - mEyePoint ).normalized();
	mOrientation = Quatf( glm::toQuat( alignZAxisWithTarget( -mViewDirection, mWorldUp ) ) );
	mModelViewCached = false;
}

void Camera::lookAt( const Vec3f &aEyePoint, const Vec3f &target )
{
	mEyePoint = aEyePoint;
	mViewDirection = ( target - mEyePoint ).normalized();
	mOrientation = Quatf( glm::toQuat( alignZAxisWithTarget( -mViewDirection, mWorldUp ) ) );
	mModelViewCached = false;
}

void Camera::lookAt( const Vec3f &aEyePoint, const Vec3f &target, const Vec3f &aWorldUp )
{
	mEyePoint = aEyePoint;
	mWorldUp = aWorldUp.normalized();
	mViewDirection = ( target - mEyePoint ).normalized();
	mOrientation = Quatf( glm::toQuat( alignZAxisWithTarget( -mViewDirection, mWorldUp ) ) );
	mModelViewCached = false;
}

void Camera::getNearClipCoordinates( Vec3f *topLeft, Vec3f *topRight, Vec3f *bottomLeft, Vec3f *bottomRight ) const
{
	calcMatrices();

	Vec3f viewDirection( mViewDirection );
	viewDirection.normalize();

	*topLeft		= mEyePoint + (mNearClip * viewDirection) + (mFrustumTop * mV) + (mFrustumLeft * mU);
	*topRight		= mEyePoint + (mNearClip * viewDirection) + (mFrustumTop * mV) + (mFrustumRight * mU);
	*bottomLeft		= mEyePoint + (mNearClip * viewDirection) + (mFrustumBottom * mV) + (mFrustumLeft * mU);
	*bottomRight	= mEyePoint + (mNearClip * viewDirection) + (mFrustumBottom * mV) + (mFrustumRight * mU);
}

void Camera::getFarClipCoordinates( Vec3f *topLeft, Vec3f *topRight, Vec3f *bottomLeft, Vec3f *bottomRight ) const
{
	calcMatrices();

	Vec3f viewDirection( mViewDirection );
	viewDirection.normalize();

	float ratio = mFarClip / mNearClip;

	*topLeft		= mEyePoint + (mFarClip * viewDirection) + (ratio * mFrustumTop * mV) + (ratio * mFrustumLeft * mU);
	*topRight		= mEyePoint + (mFarClip * viewDirection) + (ratio * mFrustumTop * mV) + (ratio * mFrustumRight * mU);
	*bottomLeft		= mEyePoint + (mFarClip * viewDirection) + (ratio * mFrustumBottom * mV) + (ratio * mFrustumLeft * mU);
	*bottomRight	= mEyePoint + (mFarClip * viewDirection) + (ratio * mFrustumBottom * mV) + (ratio * mFrustumRight * mU);
}

void Camera::getFrustum( float *left, float *top, float *right, float *bottom, float *near, float *far ) const
{
	calcMatrices();

	*left = mFrustumLeft;
	*top = mFrustumTop;
	*right = mFrustumRight;
	*bottom = mFrustumBottom;
	*near = mNearClip;
	*far = mFarClip;
}

Ray Camera::generateRay( float uPos, float vPos, float imagePlaneApectRatio ) const
{	
	calcMatrices();

	float s = ( uPos - 0.5f ) * imagePlaneApectRatio;
	float t = ( vPos - 0.5f );
	float viewDistance = imagePlaneApectRatio / math<float>::abs( mFrustumRight - mFrustumLeft ) * mNearClip;
	return Ray( mEyePoint, ( mU * s + mV * t - ( mW * viewDistance ) ).normalized() );
}

void Camera::getBillboardVectors( Vec3f *right, Vec3f *up ) const
{
	*right = fromGlm( glm::vec3( glm::column( getViewMatrix(), 0 ) ) );
	*up = fromGlm( glm::vec3( glm::column( getViewMatrix(), 1 ) ) );
}

Vec2f Camera::worldToScreen( const Vec3f &worldCoord, float screenWidth, float screenHeight ) const
{
	vec3 eyeCoord = vec3( getViewMatrix() * vec4( toGlm( worldCoord ), 1 ) );
	vec4 ndc = getProjectionMatrix() * vec4( eyeCoord, 1 );
	ndc.x /= ndc.w;
	ndc.y /= ndc.w;
	ndc.z /= ndc.w;

	return Vec2f( ( ndc.x + 1.0f ) / 2.0f * screenWidth, ( 1.0f - ( ndc.y + 1.0f ) / 2.0f ) * screenHeight );
}

float Camera::worldToEyeDepth( const Vec3f &worldCoord ) const
{
	const mat4 &m = getViewMatrix();
	return	m[0][2] * worldCoord.x +
			m[1][2] * worldCoord.y +
			m[2][2] * worldCoord.z +
			m[3][2];
}


Vec3f Camera::worldToNdc( const Vec3f &worldCoord )
{
	vec4 eye = getViewMatrix() * vec4( toGlm( worldCoord ), 1 );
	vec4 unproj = getProjectionMatrix() * eye;
	return Vec3f( unproj.x / unproj.w, unproj.y / unproj.w, unproj.z / unproj.w );
}

//* This only mostly works
float Camera::getScreenRadius( const Sphere &sphere, float screenWidth, float screenHeight ) const
{
	Vec2f screenCenter( worldToScreen( sphere.getCenter(), screenWidth, screenHeight ) );	
	Vec3f orthog = mViewDirection.getOrthogonal().normalized();
	Vec2f screenPerimeter = worldToScreen( sphere.getCenter() + sphere.getRadius() * orthog, screenWidth, screenHeight );
	return screenPerimeter.distance( screenCenter );
}

void Camera::calcMatrices() const
{
	if( ! mModelViewCached ) calcViewMatrix();
	if( ! mProjectionCached ) calcProjection();

	// note: calculation of the inverse modelview matrices is postponed until actually requested
	//if( ! mInverseModelViewCached ) calcInverseModelView();
}

void Camera::calcViewMatrix() const
{
	mW = -mViewDirection.normalized();
	mU = fromGlm( glm::rotate( mOrientation, glm::vec3( 1, 0, 0 ) ) );
	mV = fromGlm( glm::rotate( mOrientation, glm::vec3( 0, 1, 0 ) ) );
	
	Vec3f d( -mEyePoint.dot( mU ), -mEyePoint.dot( mV ), -mEyePoint.dot( mW ) );

	mat4 &m = mViewMatrix;
	m[0][0] = mU.x; m[1][0] = mU.y; m[2][0] = mU.z; m[3][0] =  d.x;
	m[0][1] = mV.x; m[1][1] = mV.y; m[2][1] = mV.z; m[3][1] =  d.y;
	m[0][2] = mW.x; m[1][2] = mW.y; m[2][2] = mW.z; m[3][2] =  d.z;
	m[0][3] = 0.0f; m[1][3] = 0.0f; m[2][3] = 0.0f; m[3][3] = 1.0f;

	mModelViewCached = true;
	mInverseModelViewCached = false;
}

void Camera::calcInverseView() const
{
	if( ! mModelViewCached ) calcViewMatrix();

	mInverseModelViewMatrix = glm::inverse( mViewMatrix );
	mInverseModelViewCached = true;
}

////////////////////////////////////////////////////////////////////////////////////////
// CameraPersp

CameraPersp::CameraPersp( int pixelWidth, int pixelHeight, float fovDegrees )
	: Camera(), mLensShift( Vec2f::zero() )
{
	float eyeX 		= pixelWidth / 2.0f;
	float eyeY 		= pixelHeight / 2.0f;
	float halfFov 	= 3.14159f * fovDegrees / 360.0f;
	float theTan 	= cinder::math<float>::tan( halfFov );
	float dist 		= eyeY / theTan;
	float nearDist 	= dist / 10.0f;	// near / far clip plane
	float farDist 	= dist * 10.0f;
	float aspect 	= pixelWidth / (float)pixelHeight;

	setPerspective( fovDegrees, aspect, nearDist, farDist );
	lookAt( Vec3f( eyeX, eyeY, dist ), Vec3f( eyeX, eyeY, 0.0f ) );
}

CameraPersp::CameraPersp( int pixelWidth, int pixelHeight, float fovDegrees, float nearPlane, float farPlane )
	: Camera(), mLensShift( Vec2f::zero() )
{
	float halfFov, theTan, aspect;

	float eyeX 		= pixelWidth / 2.0f;
	float eyeY 		= pixelHeight / 2.0f;
	halfFov 		= 3.14159f * fovDegrees / 360.0f;
	theTan 			= cinder::math<float>::tan( halfFov );
	float dist 		= eyeY / theTan;
	aspect 			= pixelWidth / (float)pixelHeight;

	setPerspective( fovDegrees, aspect, nearPlane, farPlane );
	lookAt( Vec3f( eyeX, eyeY, dist ), Vec3f( eyeX, eyeY, 0.0f ) );
}

// Creates a default camera resembling Maya Persp
CameraPersp::CameraPersp()
	: Camera(), mLensShift( Vec2f::zero() )
{
	lookAt( Vec3f( 28.0f, 21.0f, 28.0f ), Vec3f::zero(), Vec3f::yAxis() );
	setCenterOfInterest( 44.822f );
	setPerspective( 35.0f, 1.0f, 0.1f, 1000.0f );
}

void CameraPersp::setPerspective( float verticalFovDegrees, float aspectRatio, float nearPlane, float farPlane )
{
	mFov			= verticalFovDegrees;
	mAspectRatio	= aspectRatio;
	mNearClip		= nearPlane;
	mFarClip		= farPlane;

	mProjectionCached = false;
}

void CameraPersp::calcProjection() const
{
	mFrustumTop		=  mNearClip * math<float>::tan( (float)M_PI / 180.0f * mFov * 0.5f );
	mFrustumBottom	= -mFrustumTop;
	mFrustumRight	=  mFrustumTop * mAspectRatio;
	mFrustumLeft	= -mFrustumRight;

	// perform lens shift
	if( mLensShift.y != 0.0f ) {
		mFrustumTop = ci::lerp<float, float>(0.0f, 2.0f * mFrustumTop, 0.5f + 0.5f * mLensShift.y);
		mFrustumBottom = ci::lerp<float, float>(2.0f * mFrustumBottom, 0.0f, 0.5f + 0.5f * mLensShift.y);
	}

	if( mLensShift.x != 0.0f ) {
		mFrustumRight = ci::lerp<float, float>(2.0f * mFrustumRight, 0.0f, 0.5f - 0.5f * mLensShift.x);
		mFrustumLeft = ci::lerp<float, float>(0.0f, 2.0f * mFrustumLeft, 0.5f - 0.5f * mLensShift.x);
	}

	mat4 &p = mProjectionMatrix;
	p[0][0] =  2.0f * mNearClip / ( mFrustumRight - mFrustumLeft );
	p[1][0] =  0.0f;
	p[2][0] =  ( mFrustumRight + mFrustumLeft ) / ( mFrustumRight - mFrustumLeft );
	p[3][0] =  0.0f;

	p[0][1] =  0.0f;
	p[1][1] =  2.0f * mNearClip / ( mFrustumTop - mFrustumBottom );
	p[2][1] =  ( mFrustumTop + mFrustumBottom ) / ( mFrustumTop - mFrustumBottom );
	p[3][1] =  0.0f;

	p[0][2] =  0.0f;
	p[1][2] =  0.0f;
	p[2][2] = -( mFarClip + mNearClip ) / ( mFarClip - mNearClip );
	p[3][2] = -2.0f * mFarClip * mNearClip / ( mFarClip - mNearClip );

	p[0][3] =  0.0f;
	p[1][3] =  0.0f;
	p[2][3] = -1.0f;
	p[3][3] =  0.0f;

	mat4 &m = mInverseProjectionMatrix;
	m[0][0] =  ( mFrustumRight - mFrustumLeft ) / ( 2.0f * mNearClip );
	m[1][0] =  0.0f;
	m[2][0] =  0.0f;
	m[3][0] =  ( mFrustumRight + mFrustumLeft ) / ( 2.0f * mNearClip );

	m[0][1] =  0.0f;
	m[1][1] =  ( mFrustumTop - mFrustumBottom ) / ( 2.0f * mNearClip );
	m[2][1] =  0.0f;
	m[3][1] =  ( mFrustumTop + mFrustumBottom ) / ( 2.0f * mNearClip );

	m[0][2] =  0.0f;
	m[1][2] =  0.0f;
	m[2][2] =  0.0f;
	m[3][2] = -1.0f;

	m[0][3] =  0.0f;
	m[1][3] =  0.0f;
	m[2][3] = -( mFarClip - mNearClip ) / ( 2.0f * mFarClip*mNearClip );
	m[3][3] =  ( mFarClip + mNearClip ) / ( 2.0f * mFarClip*mNearClip );

	mProjectionCached = true;
}

void CameraPersp::setLensShift(float horizontal, float vertical)
{
	mLensShift.x = horizontal;
	mLensShift.y = vertical;

	mProjectionCached = false;
}

CameraPersp	CameraPersp::getFrameSphere( const Sphere &worldSpaceSphere, int maxIterations ) const
{
	CameraPersp result = *this;
	result.setEyePoint( worldSpaceSphere.getCenter() - result.mViewDirection * getCenterOfInterest() );
	
	float minDistance = 0.01f, maxDistance = 100000.0f;
	float curDistance = getCenterOfInterest();
	for( int i = 0; i < maxIterations; ++i ) {
		float curRadius = result.getScreenRadius( worldSpaceSphere, 2.0f, 2.0f );
		if( curRadius < 1.0f ) { // we should get closer
			maxDistance = curDistance;
			curDistance = ( curDistance + minDistance ) * 0.5f;
		}
		else { // we should get farther
			minDistance = curDistance;
			curDistance = ( curDistance + maxDistance ) * 0.5f;			
		}
		result.setEyePoint( worldSpaceSphere.getCenter() - result.mViewDirection * curDistance );
	}
	
	result.setCenterOfInterest( result.getEyePoint().distance( worldSpaceSphere.getCenter() ) );
	return result;
}

////////////////////////////////////////////////////////////////////////////////////////
// CameraOrtho
CameraOrtho::CameraOrtho()
	: Camera()
{
	lookAt( Vec3f( 0.0f, 0.0f, 0.1f ), Vec3f::zero(), Vec3f::yAxis() );
	setCenterOfInterest( 0.1f );
	setFov( 35.0f );
}

CameraOrtho::CameraOrtho( float left, float right, float bottom, float top, float nearPlane, float farPlane )
	: Camera()
{
	mFrustumLeft	= left;
	mFrustumRight	= right;
	mFrustumTop		= top;
	mFrustumBottom	= bottom;
	mNearClip		= nearPlane;
	mFarClip		= farPlane;
	
	mProjectionCached = false;
	mModelViewCached = true;
	mInverseModelViewCached = true;
}

void CameraOrtho::setOrtho( float left, float right, float bottom, float top, float nearPlane, float farPlane )
{
	mFrustumLeft	= left;
	mFrustumRight	= right;
	mFrustumTop		= top;
	mFrustumBottom	= bottom;
	mNearClip		= nearPlane;
	mFarClip		= farPlane;

	mProjectionCached = false;
}

void CameraOrtho::calcProjection() const
{
	mat4 &p = mProjectionMatrix;
	p[0][0] =  2.0f/(mFrustumRight - mFrustumLeft);
	p[1][0] =  0.0f;
	p[2][0] =  0.0f;
	p[3][0] =  -(mFrustumRight + mFrustumLeft)/(mFrustumRight - mFrustumLeft);

	p[0][1] =  0.0f;
	p[1][1] =  2.0f/(mFrustumTop - mFrustumBottom);
	p[2][1] =  0.0f;
	p[3][1] =  -(mFrustumTop + mFrustumBottom)/(mFrustumTop - mFrustumBottom);

	p[0][2] =  0.0f;
	p[1][2] =  0.0f;
	p[2][2] = -2.0f/(mFarClip - mNearClip);
	p[3][2] = -(mFarClip + mNearClip)/(mFarClip - mNearClip);

	p[0][3] =  0.0f;
	p[1][3] =  0.0f;
	p[2][3] =  0.0f;
	p[3][3] =  1.0f;

	mat4 &m = mInverseProjectionMatrix;
	m[0][0] =  (mFrustumRight - mFrustumLeft) * 0.5f;
	m[1][0] =  0.0f;
	m[2][0] =  0.0f;
	m[3][0] =  (mFrustumRight + mFrustumLeft) * 0.5f;

	m[0][1] =  0.0f;
	m[1][1] =  (mFrustumTop - mFrustumBottom) * 0.5f;
	m[2][1] =  0.0f;
	m[3][1] =  (mFrustumTop + mFrustumBottom) * 0.5f;

	m[0][2] =  0.0f;
	m[1][2] =  0.0f;
	m[2][2] =  (mFarClip - mNearClip) * 0.5f;
	m[3][2] =  (mNearClip + mFarClip) * 0.5f;

	m[0][3] =  0.0f;
	m[1][3] =  0.0f;
	m[2][3] =  0.0f;
	m[3][3] =  1.0f;

	mProjectionCached = true;
}

////////////////////////////////////////////////////////////////////////////////////////
// CameraStereo

Vec3f CameraStereo::getEyePointShifted() const
{	
	if(!mIsStereo)
		return mEyePoint;
	
	if(mIsLeft) 
		return mEyePoint - fromGlm( glm::rotate( mOrientation, vec3( 1, 0, 0 ) ) ) * (0.5f * mEyeSeparation);
	else 
		return mEyePoint + fromGlm( glm::rotate( mOrientation, vec3( 1, 0, 0 ) ) ) * (0.5f * mEyeSeparation);
}

void CameraStereo::getNearClipCoordinates( Vec3f *topLeft, Vec3f *topRight, Vec3f *bottomLeft, Vec3f *bottomRight ) const
{
	calcMatrices();

	Vec3f viewDirection( mViewDirection );
	viewDirection.normalize();

	Vec3f eye( getEyePointShifted() );

	float shift = 0.5f * mEyeSeparation * (mNearClip / mConvergence);
	shift *= (mIsStereo ? (mIsLeft ? 1.0f : -1.0f) : 0.0f);

	float left = mFrustumLeft + shift;
	float right = mFrustumRight + shift;

	*topLeft		= eye + (mNearClip * viewDirection) + (mFrustumTop * mV) + (left * mU);
	*topRight		= eye + (mNearClip * viewDirection) + (mFrustumTop * mV) + (right * mU);
	*bottomLeft		= eye + (mNearClip * viewDirection) + (mFrustumBottom * mV) + (left * mU);
	*bottomRight	= eye + (mNearClip * viewDirection) + (mFrustumBottom * mV) + (right * mU);
}

void CameraStereo::getFarClipCoordinates( Vec3f *topLeft, Vec3f *topRight, Vec3f *bottomLeft, Vec3f *bottomRight ) const
{
	calcMatrices();

	Vec3f viewDirection( mViewDirection );
	viewDirection.normalize();

	float ratio = mFarClip / mNearClip;

	Vec3f eye( getEyePointShifted() );

	float shift = 0.5f * mEyeSeparation * (mNearClip / mConvergence);
	shift *= (mIsStereo ? (mIsLeft ? 1.0f : -1.0f) : 0.0f);

	float left = mFrustumLeft + shift;
	float right = mFrustumRight + shift;

	*topLeft		= eye + (mFarClip * viewDirection) + (ratio * mFrustumTop * mV) + (ratio * left * mU);
	*topRight		= eye + (mFarClip * viewDirection) + (ratio * mFrustumTop * mV) + (ratio * right * mU);
	*bottomLeft		= eye + (mFarClip * viewDirection) + (ratio * mFrustumBottom * mV) + (ratio * left * mU);
	*bottomRight	= eye + (mFarClip * viewDirection) + (ratio * mFrustumBottom * mV) + (ratio * right * mU);
}
	
const Matrix44f& CameraStereo::getProjectionMatrix() const 
{
	if( ! mProjectionCached )
		calcProjection(); 

	if( ! mIsStereo )
		return mProjectionMatrix; 
	else if( mIsLeft )
		return mProjectionMatrixLeft; 
	else
		return mProjectionMatrixRight; 
}
	
const Matrix44f& CameraStereo::getViewMatrix() const 
{
	if( ! mModelViewCached )
		calcViewMatrix(); 

	if( ! mIsStereo )
		return mViewMatrix; 
	else if( mIsLeft )
		return mViewMatrixLeft; 
	else
		return mViewMatrixRight; 
}
	
const Matrix44f& CameraStereo::getInverseViewMatrix() const 
{
	if( ! mInverseModelViewCached )
		calcInverseView();

	if( ! mIsStereo )
		return mInverseModelViewMatrix; 
	else if( mIsLeft )
		return mInverseModelViewMatrixLeft; 
	else
		return mInverseModelViewMatrixRight; 
}

void CameraStereo::calcViewMatrix() const
{
	// calculate default matrix first
	CameraPersp::calcViewMatrix();
	
	mViewMatrixLeft = mViewMatrix;
	mViewMatrixRight = mViewMatrix;
	
	// calculate left matrix
	Vec3f eye = mEyePoint - fromGlm( glm::rotate( mOrientation, vec3( 1, 0, 0 ) ) ) * (0.5f * mEyeSeparation);
	Vec3f d = Vec3f( -eye.dot( mU ), -eye.dot( mV ), -eye.dot( mW ) );

	mViewMatrixLeft[3][0] = d.x; mViewMatrixLeft[3][1] = d.y; mViewMatrixLeft[3][2] = d.z;

	// calculate right matrix
	eye = mEyePoint + fromGlm( glm::rotate( mOrientation, vec3( 1, 0, 0 ) ) ) * (0.5f * mEyeSeparation);
	d = Vec3f( -eye.dot( mU ), -eye.dot( mV ), -eye.dot( mW ) );

	mViewMatrixRight[3][0] =  d.x; mViewMatrixRight[3][1] = d.y; mViewMatrixRight[3][2] = d.z;

	mModelViewCached = true;
	mInverseModelViewCached = false;
}

void CameraStereo::calcInverseView() const
{
	if( ! mModelViewCached ) calcViewMatrix();

	mInverseModelViewMatrix = glm::affineInverse( mViewMatrix );
	mInverseModelViewMatrixLeft = glm::affineInverse( mViewMatrixLeft );
	mInverseModelViewMatrixRight = glm::affineInverse( mViewMatrixRight );
	mInverseModelViewCached = true;
}

void CameraStereo::calcProjection() const
{
	// calculate default matrices first
	CameraPersp::calcProjection();
	
	mProjectionMatrixLeft = mProjectionMatrix;
	mInverseProjectionMatrixLeft = mInverseProjectionMatrix;
	
	mProjectionMatrixRight = mProjectionMatrix;
	mInverseProjectionMatrixRight = mInverseProjectionMatrix;

	// calculate left matrices
	mInverseProjectionMatrixLeft[2][0] =  ( mFrustumRight + mFrustumLeft + mEyeSeparation * (mNearClip / mConvergence) ) / ( mFrustumRight - mFrustumLeft );

	mInverseProjectionMatrixLeft[3][0] =  ( mFrustumRight + mFrustumLeft + mEyeSeparation * (mNearClip / mConvergence) ) / ( 2.0f * mNearClip );	

	// calculate right matrices
	mProjectionMatrixRight[2][0] =  ( mFrustumRight + mFrustumLeft - mEyeSeparation * (mNearClip / mConvergence) ) / ( mFrustumRight - mFrustumLeft );

	mProjectionMatrixRight[3][0] =  ( mFrustumRight + mFrustumLeft - mEyeSeparation * (mNearClip / mConvergence) ) / ( 2.0f * mNearClip );
	
	mProjectionCached = true;
}

} // namespace cinder

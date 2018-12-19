//--------------------------------------------------------------------------------------
// File: DXUTcamera.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=320437
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTres.h"

using namespace DirectX;

//======================================================================================
// CD3DArcBall
//======================================================================================

//--------------------------------------------------------------------------------------
CD3DArcBall::CD3DArcBall()
{
    Reset();

    m_vDownPt = XMFLOAT3( 0, 0, 0 );
    m_vCurrentPt = XMFLOAT3( 0, 0, 0 );
    m_Offset.x = m_Offset.y = 0;

    RECT rc;
    GetClientRect( GetForegroundWindow(), &rc );
    SetWindow( rc.right, rc.bottom );
}


//--------------------------------------------------------------------------------------
void CD3DArcBall::Reset()
{
    XMVECTOR qid = XMQuaternionIdentity();
    XMStoreFloat4( &m_qDown, qid );
    XMStoreFloat4( &m_qNow, qid );

    XMMATRIX id = XMMatrixIdentity();
    XMStoreFloat4x4( &m_mRotation, id );
    XMStoreFloat4x4( &m_mTranslation, id );
    XMStoreFloat4x4( &m_mTranslationDelta, id );

    m_bDrag = false;
    m_fRadiusTranslation = 1.0f;
    m_fRadius = 1.0f;
}


//--------------------------------------------------------------------------------------
void CD3DArcBall::OnBegin( _In_ int nX, _In_ int nY )
{
    // Only enter the drag state if the click falls
    // inside the click rectangle.
    if( nX >= m_Offset.x &&
        nX < m_Offset.x + m_nWidth &&
        nY >= m_Offset.y &&
        nY < m_Offset.y + m_nHeight )
    {
        m_bDrag = true;
        m_qDown = m_qNow;
        XMVECTOR v = ScreenToVector( float(nX), float(nY) );
        XMStoreFloat3( &m_vDownPt, v );
    }
}


//--------------------------------------------------------------------------------------
void CD3DArcBall::OnMove( _In_ int nX, _In_ int nY )
{
    if( m_bDrag )
    {
        XMVECTOR curr = ScreenToVector( ( float )nX, ( float )nY ); 
        XMStoreFloat3( &m_vCurrentPt, curr );

        XMVECTOR down = XMLoadFloat3( &m_vDownPt );
        XMVECTOR qdown = XMLoadFloat4( &m_qDown );

        XMVECTOR result = XMQuaternionMultiply( qdown, QuatFromBallPoints( down, curr ) );
        XMStoreFloat4( &m_qNow, result );
    }
}


//--------------------------------------------------------------------------------------
void CD3DArcBall::OnEnd()
{
    m_bDrag = false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
LRESULT CD3DArcBall::HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    // Current mouse position
    int iMouseX = ( short )LOWORD( lParam );
    int iMouseY = ( short )HIWORD( lParam );

    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            SetCapture( hWnd );
            OnBegin( iMouseX, iMouseY );
            return TRUE;

        case WM_LBUTTONUP:
            ReleaseCapture();
            OnEnd();
            return TRUE;
        case WM_CAPTURECHANGED:
            if( ( HWND )lParam != hWnd )
            {
                ReleaseCapture();
                OnEnd();
            }
            return TRUE;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
            SetCapture( hWnd );
            // Store off the position of the cursor when the button is pressed
            m_ptLastMouse.x = iMouseX;
            m_ptLastMouse.y = iMouseY;
            return TRUE;

        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            ReleaseCapture();
            return TRUE;

        case WM_MOUSEMOVE:
            if( MK_LBUTTON & wParam )
            {
                OnMove( iMouseX, iMouseY );
            }
            else if( ( MK_RBUTTON & wParam ) || ( MK_MBUTTON & wParam ) )
            {
                // Normalize based on size of window and bounding sphere radius
                float fDeltaX = ( m_ptLastMouse.x - iMouseX ) * m_fRadiusTranslation / m_nWidth;
                float fDeltaY = ( m_ptLastMouse.y - iMouseY ) * m_fRadiusTranslation / m_nHeight;

                XMMATRIX mTranslationDelta;
                XMMATRIX mTranslation = XMLoadFloat4x4( &m_mTranslation );
                if( wParam & MK_RBUTTON )
                {
                    mTranslationDelta = XMMatrixTranslation( -2 * fDeltaX, 2 * fDeltaY, 0.0f );
                    mTranslation = XMMatrixMultiply( mTranslation, mTranslationDelta );
                }
                else  // wParam & MK_MBUTTON
                {
                    mTranslationDelta = XMMatrixTranslation( 0.0f, 0.0f, 5 * fDeltaY );
                    mTranslation = XMMatrixMultiply( mTranslation, mTranslationDelta );
                }

                XMStoreFloat4x4( &m_mTranslationDelta, mTranslationDelta );
                XMStoreFloat4x4( &m_mTranslation, mTranslation );

                // Store mouse coordinate
                m_ptLastMouse.x = iMouseX;
                m_ptLastMouse.y = iMouseY;
            }
            return TRUE;
    }

    return FALSE;
}


//======================================================================================
// CBaseCamera
//======================================================================================

//--------------------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------------------
CBaseCamera::CBaseCamera() :
    m_cKeysDown(0),
    m_nCurrentButtonMask(0),
    m_nMouseWheelDelta(0),
    m_fFramesToSmoothMouseData(2.0f),
    m_fCameraYawAngle(0.0f),
    m_fCameraPitchAngle(0.0f),
    m_fDragTimer(0.0f),
    m_fTotalDragTimeToZero(0.25),
    m_fRotationScaler(0.01f),
    m_fMoveScaler(5.0f),
    m_bMouseLButtonDown(false),
    m_bMouseMButtonDown(false),
    m_bMouseRButtonDown(false),
    m_bMovementDrag(false),
    m_bInvertPitch(false),
    m_bEnablePositionMovement(true),
    m_bEnableYAxisMovement(true),
    m_bClipToBoundary(false),
    m_bResetCursorAfterMove(false)
{
    ZeroMemory( m_aKeys, sizeof( BYTE ) * CAM_MAX_KEYS );
    ZeroMemory( m_GamePad, sizeof( DXUT_GAMEPAD ) * DXUT_MAX_CONTROLLERS );

    // Setup the view matrix
    SetViewParams( g_XMZero, g_XMIdentityR2 );

    // Setup the projection matrix
    SetProjParams( XM_PI / 4, 1.0f, 1.0f, 1000.0f );

    GetCursorPos( &m_ptLastMousePosition );
    
    SetRect( &m_rcDrag, LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX );
    m_vVelocity = XMFLOAT3( 0, 0, 0 );
    m_vVelocityDrag = XMFLOAT3( 0, 0, 0 );
    m_vRotVelocity = XMFLOAT2( 0, 0 );

    m_vMouseDelta = XMFLOAT2( 0, 0 );

    m_vMinBoundary = XMFLOAT3( -1, -1, -1 );
    m_vMaxBoundary = XMFLOAT3( 1, 1, 1 );
}


//--------------------------------------------------------------------------------------
// Client can call this to change the position and direction of camera
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CBaseCamera::SetViewParams( FXMVECTOR vEyePt, FXMVECTOR vLookatPt )
{
    XMStoreFloat3( &m_vEye, vEyePt );
    XMStoreFloat3( &m_vDefaultEye, vEyePt );

    XMStoreFloat3( &m_vLookAt, vLookatPt );
    XMStoreFloat3( &m_vDefaultLookAt , vLookatPt );

    // Calc the view matrix
    XMMATRIX mView = XMMatrixLookAtLH( vEyePt, vLookatPt, g_XMIdentityR1 );
    XMStoreFloat4x4( &m_mView, mView );

    XMMATRIX mInvView = XMMatrixInverse( nullptr, mView );

    // The axis basis vectors and camera position are stored inside the 
    // position matrix in the 4 rows of the camera's world matrix.
    // To figure out the yaw/pitch of the camera, we just need the Z basis vector
    XMFLOAT3 zBasis;
    XMStoreFloat3( &zBasis, mInvView.r[2] );

    m_fCameraYawAngle = atan2f( zBasis.x, zBasis.z );
    float fLen = sqrtf( zBasis.z * zBasis.z + zBasis.x * zBasis.x );
    m_fCameraPitchAngle = -atan2f( zBasis.y, fLen );
}


//--------------------------------------------------------------------------------------
// Calculates the projection matrix based on input params
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CBaseCamera::SetProjParams( float fFOV, float fAspect, float fNearPlane, float fFarPlane )
{
    // Set attributes for the projection matrix
    m_fFOV = fFOV;
    m_fAspect = fAspect;
    m_fNearPlane = fNearPlane;
    m_fFarPlane = fFarPlane;

    XMMATRIX mProj = XMMatrixPerspectiveFovLH( fFOV, fAspect, fNearPlane, fFarPlane );
    XMStoreFloat4x4( &m_mProj, mProj );
}


//--------------------------------------------------------------------------------------
// Call this from your message proc so this class can handle window messages
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
LRESULT CBaseCamera::HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( hWnd );
    UNREFERENCED_PARAMETER( lParam );

    switch( uMsg )
    {
        case WM_KEYDOWN:
        {
            // Map this key to a D3DUtil_CameraKeys enum and update the
            // state of m_aKeys[] by adding the KEY_WAS_DOWN_MASK|KEY_IS_DOWN_MASK mask
            // only if the key is not down
            D3DUtil_CameraKeys mappedKey = MapKey( ( UINT )wParam );
            if( mappedKey != CAM_UNKNOWN )
            {
                _Analysis_assume_( mappedKey < CAM_MAX_KEYS );
                if( FALSE == IsKeyDown( m_aKeys[mappedKey] ) )
                {
                    m_aKeys[ mappedKey ] = KEY_WAS_DOWN_MASK | KEY_IS_DOWN_MASK;
                    ++m_cKeysDown;
                }
            }
            break;
        }

        case WM_KEYUP:
        {
            // Map this key to a D3DUtil_CameraKeys enum and update the
            // state of m_aKeys[] by removing the KEY_IS_DOWN_MASK mask.
            D3DUtil_CameraKeys mappedKey = MapKey( ( UINT )wParam );
            if( mappedKey != CAM_UNKNOWN && ( DWORD )mappedKey < 8 )
            {
                m_aKeys[ mappedKey ] &= ~KEY_IS_DOWN_MASK;
                --m_cKeysDown;
            }
            break;
        }

        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
            {
                // Compute the drag rectangle in screen coord.
                POINT ptCursor =
                {
                    ( short )LOWORD( lParam ), ( short )HIWORD( lParam )
                };

                // Update member var state
                if( ( uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK ) && PtInRect( &m_rcDrag, ptCursor ) )
                {
                    m_bMouseLButtonDown = true; m_nCurrentButtonMask |= MOUSE_LEFT_BUTTON;
                }
                if( ( uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONDBLCLK ) && PtInRect( &m_rcDrag, ptCursor ) )
                {
                    m_bMouseMButtonDown = true; m_nCurrentButtonMask |= MOUSE_MIDDLE_BUTTON;
                }
                if( ( uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONDBLCLK ) && PtInRect( &m_rcDrag, ptCursor ) )
                {
                    m_bMouseRButtonDown = true; m_nCurrentButtonMask |= MOUSE_RIGHT_BUTTON;
                }

                // Capture the mouse, so if the mouse button is 
                // released outside the window, we'll get the WM_LBUTTONUP message
                SetCapture( hWnd );
                GetCursorPos( &m_ptLastMousePosition );
                return TRUE;
            }

        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_LBUTTONUP:
            {
                // Update member var state
                if( uMsg == WM_LBUTTONUP )
                {
                    m_bMouseLButtonDown = false; m_nCurrentButtonMask &= ~MOUSE_LEFT_BUTTON;
                }
                if( uMsg == WM_MBUTTONUP )
                {
                    m_bMouseMButtonDown = false; m_nCurrentButtonMask &= ~MOUSE_MIDDLE_BUTTON;
                }
                if( uMsg == WM_RBUTTONUP )
                {
                    m_bMouseRButtonDown = false; m_nCurrentButtonMask &= ~MOUSE_RIGHT_BUTTON;
                }

                // Release the capture if no mouse buttons down
                if( !m_bMouseLButtonDown &&
                    !m_bMouseRButtonDown &&
                    !m_bMouseMButtonDown )
                {
                    ReleaseCapture();
                }
                break;
            }

        case WM_CAPTURECHANGED:
        {
            if( ( HWND )lParam != hWnd )
            {
                if( ( m_nCurrentButtonMask & MOUSE_LEFT_BUTTON ) ||
                    ( m_nCurrentButtonMask & MOUSE_MIDDLE_BUTTON ) ||
                    ( m_nCurrentButtonMask & MOUSE_RIGHT_BUTTON ) )
                {
                    m_bMouseLButtonDown = false;
                    m_bMouseMButtonDown = false;
                    m_bMouseRButtonDown = false;
                    m_nCurrentButtonMask &= ~MOUSE_LEFT_BUTTON;
                    m_nCurrentButtonMask &= ~MOUSE_MIDDLE_BUTTON;
                    m_nCurrentButtonMask &= ~MOUSE_RIGHT_BUTTON;
                    ReleaseCapture();
                }
            }
            break;
        }

        case WM_MOUSEWHEEL:
            // Update member var state
            m_nMouseWheelDelta += ( short )HIWORD( wParam );
            break;
    }

    return FALSE;
}


//--------------------------------------------------------------------------------------
// Figure out the velocity based on keyboard input & drag if any
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CBaseCamera::GetInput( bool bGetKeyboardInput, bool bGetMouseInput, bool bGetGamepadInput )
{
    m_vKeyboardDirection = XMFLOAT3( 0, 0, 0 );
    if( bGetKeyboardInput )
    {
        // Update acceleration vector based on keyboard state
        if( IsKeyDown( m_aKeys[CAM_MOVE_FORWARD] ) )
            m_vKeyboardDirection.z += 1.0f;
        if( IsKeyDown( m_aKeys[CAM_MOVE_BACKWARD] ) )
            m_vKeyboardDirection.z -= 1.0f;
        if( m_bEnableYAxisMovement )
        {
            if( IsKeyDown( m_aKeys[CAM_MOVE_UP] ) )
                m_vKeyboardDirection.y += 1.0f;
            if( IsKeyDown( m_aKeys[CAM_MOVE_DOWN] ) )
                m_vKeyboardDirection.y -= 1.0f;
        }
        if( IsKeyDown( m_aKeys[CAM_STRAFE_RIGHT] ) )
            m_vKeyboardDirection.x += 1.0f;
        if( IsKeyDown( m_aKeys[CAM_STRAFE_LEFT] ) )
            m_vKeyboardDirection.x -= 1.0f;
    }

    if( bGetMouseInput )
    {
        UpdateMouseDelta();
    }

    if( bGetGamepadInput )
    {
        m_vGamePadLeftThumb = XMFLOAT3( 0, 0, 0 );
        m_vGamePadRightThumb = XMFLOAT3( 0, 0, 0 );

        // Get controller state
        for( DWORD iUserIndex = 0; iUserIndex < DXUT_MAX_CONTROLLERS; iUserIndex++ )
        {
            DXUTGetGamepadState( iUserIndex, &m_GamePad[iUserIndex], true, true );

            // Mark time if the controller is in a non-zero state
            if( m_GamePad[iUserIndex].wButtons ||
                m_GamePad[iUserIndex].sThumbLX || m_GamePad[iUserIndex].sThumbLY ||
                m_GamePad[iUserIndex].sThumbRX || m_GamePad[iUserIndex].sThumbRY ||
                m_GamePad[iUserIndex].bLeftTrigger || m_GamePad[iUserIndex].bRightTrigger )
            {
                m_GamePadLastActive[iUserIndex] = DXUTGetTime();
            }
        }

        // Find out which controller was non-zero last
        int iMostRecentlyActive = -1;
        double fMostRecentlyActiveTime = 0.0f;
        for( DWORD iUserIndex = 0; iUserIndex < DXUT_MAX_CONTROLLERS; iUserIndex++ )
        {
            if( m_GamePadLastActive[iUserIndex] > fMostRecentlyActiveTime )
            {
                fMostRecentlyActiveTime = m_GamePadLastActive[iUserIndex];
                iMostRecentlyActive = iUserIndex;
            }
        }

        // Use the most recent non-zero controller if its connected
        if( iMostRecentlyActive >= 0 && m_GamePad[iMostRecentlyActive].bConnected )
        {
            m_vGamePadLeftThumb.x = m_GamePad[iMostRecentlyActive].fThumbLX;
            m_vGamePadLeftThumb.y = 0.0f;
            m_vGamePadLeftThumb.z = m_GamePad[iMostRecentlyActive].fThumbLY;

            m_vGamePadRightThumb.x = m_GamePad[iMostRecentlyActive].fThumbRX;
            m_vGamePadRightThumb.y = 0.0f;
            m_vGamePadRightThumb.z = m_GamePad[iMostRecentlyActive].fThumbRY;
        }
    }
}


//--------------------------------------------------------------------------------------
// Figure out the mouse delta based on mouse movement
//--------------------------------------------------------------------------------------
void CBaseCamera::UpdateMouseDelta()
{
    // Get current position of mouse
    POINT ptCurMousePos;
    GetCursorPos( &ptCurMousePos );

    // Calc how far it's moved since last frame
    POINT ptCurMouseDelta;
    ptCurMouseDelta.x = ptCurMousePos.x - m_ptLastMousePosition.x;
    ptCurMouseDelta.y = ptCurMousePos.y - m_ptLastMousePosition.y;

    // Record current position for next time
    m_ptLastMousePosition = ptCurMousePos;

    if( m_bResetCursorAfterMove && DXUTIsActive() )
    {
        // Set position of camera to center of desktop, 
        // so it always has room to move.  This is very useful
        // if the cursor is hidden.  If this isn't done and cursor is hidden, 
        // then invisible cursor will hit the edge of the screen 
        // and the user can't tell what happened
        POINT ptCenter;

        // Get the center of the current monitor
        MONITORINFO mi;
        mi.cbSize = sizeof( MONITORINFO );
        DXUTGetMonitorInfo( DXUTMonitorFromWindow( DXUTGetHWND(), MONITOR_DEFAULTTONEAREST ), &mi );
        ptCenter.x = ( mi.rcMonitor.left + mi.rcMonitor.right ) / 2;
        ptCenter.y = ( mi.rcMonitor.top + mi.rcMonitor.bottom ) / 2;
        SetCursorPos( ptCenter.x, ptCenter.y );
        m_ptLastMousePosition = ptCenter;
    }

    // Smooth the relative mouse data over a few frames so it isn't 
    // jerky when moving slowly at low frame rates.
    float fPercentOfNew = 1.0f / m_fFramesToSmoothMouseData;
    float fPercentOfOld = 1.0f - fPercentOfNew;
    m_vMouseDelta.x = m_vMouseDelta.x * fPercentOfOld + ptCurMouseDelta.x * fPercentOfNew;
    m_vMouseDelta.y = m_vMouseDelta.y * fPercentOfOld + ptCurMouseDelta.y * fPercentOfNew;

    m_vRotVelocity.x = m_vMouseDelta.x * m_fRotationScaler;
    m_vRotVelocity.y = m_vMouseDelta.y * m_fRotationScaler;
}


//--------------------------------------------------------------------------------------
// Figure out the velocity based on keyboard input & drag if any
//--------------------------------------------------------------------------------------
void CBaseCamera::UpdateVelocity( _In_ float fElapsedTime )
{
    XMVECTOR vGamePadRightThumb = XMVectorSet( m_vGamePadRightThumb.x, -m_vGamePadRightThumb.z, 0, 0 );

    XMVECTOR vMouseDelta = XMLoadFloat2( &m_vMouseDelta );
    XMVECTOR vRotVelocity = vMouseDelta * m_fRotationScaler + vGamePadRightThumb * 0.02f;

    XMStoreFloat2( &m_vRotVelocity, vRotVelocity );
    
    XMVECTOR vKeyboardDirection = XMLoadFloat3( &m_vKeyboardDirection );
    XMVECTOR vGamePadLeftThumb = XMLoadFloat3( &m_vGamePadLeftThumb );
    XMVECTOR vAccel = vKeyboardDirection + vGamePadLeftThumb;

    // Normalize vector so if moving 2 dirs (left & forward), 
    // the camera doesn't move faster than if moving in 1 dir
    vAccel = XMVector3Normalize( vAccel );

    // Scale the acceleration vector
    vAccel *= m_fMoveScaler;

    if( m_bMovementDrag )
    {
        // Is there any acceleration this frame?
        if( XMVectorGetX( XMVector3LengthSq( vAccel ) ) > 0 )
        {
            // If so, then this means the user has pressed a movement key
            // so change the velocity immediately to acceleration 
            // upon keyboard input.  This isn't normal physics
            // but it will give a quick response to keyboard input
            XMStoreFloat3( &m_vVelocity, vAccel );

            m_fDragTimer = m_fTotalDragTimeToZero;
            
            XMStoreFloat3( &m_vVelocityDrag, vAccel / m_fDragTimer );
        }
        else
        {
            // If no key being pressed, then slowly decrease velocity to 0
            if( m_fDragTimer > 0 )
            {
                // Drag until timer is <= 0
                XMVECTOR vVelocity = XMLoadFloat3( &m_vVelocity );
                XMVECTOR vVelocityDrag = XMLoadFloat3( &m_vVelocityDrag );

                vVelocity -= vVelocityDrag * fElapsedTime;

                XMStoreFloat3( &m_vVelocity, vVelocity );

                m_fDragTimer -= fElapsedTime;
            }
            else
            {
                // Zero velocity
                m_vVelocity = XMFLOAT3( 0, 0, 0 );
            }
        }
    }
    else
    {
        // No drag, so immediately change the velocity
        XMStoreFloat3( &m_vVelocity, vAccel );
    }
}


//--------------------------------------------------------------------------------------
// Maps a windows virtual key to an enum
//--------------------------------------------------------------------------------------
D3DUtil_CameraKeys CBaseCamera::MapKey( _In_ UINT nKey )
{
    // This could be upgraded to a method that's user-definable but for 
    // simplicity, we'll use a hardcoded mapping.
    switch( nKey )
    {
        case VK_CONTROL:
            return CAM_CONTROLDOWN;
        case VK_LEFT:
            return CAM_STRAFE_LEFT;
        case VK_RIGHT:
            return CAM_STRAFE_RIGHT;
        case VK_UP:
            return CAM_MOVE_FORWARD;
        case VK_DOWN:
            return CAM_MOVE_BACKWARD;
        case VK_PRIOR:
            return CAM_MOVE_UP;        // pgup
        case VK_NEXT:
            return CAM_MOVE_DOWN;      // pgdn

        case 'A':
            return CAM_STRAFE_LEFT;
        case 'D':
            return CAM_STRAFE_RIGHT;
        case 'W':
            return CAM_MOVE_FORWARD;
        case 'S':
            return CAM_MOVE_BACKWARD;
        case 'Q':
            return CAM_MOVE_DOWN;
        case 'E':
            return CAM_MOVE_UP;

        case VK_NUMPAD4:
            return CAM_STRAFE_LEFT;
        case VK_NUMPAD6:
            return CAM_STRAFE_RIGHT;
        case VK_NUMPAD8:
            return CAM_MOVE_FORWARD;
        case VK_NUMPAD2:
            return CAM_MOVE_BACKWARD;
        case VK_NUMPAD9:
            return CAM_MOVE_UP;
        case VK_NUMPAD3:
            return CAM_MOVE_DOWN;

        case VK_HOME:
            return CAM_RESET;
    }

    return CAM_UNKNOWN;
}


//--------------------------------------------------------------------------------------
// Reset the camera's position back to the default
//--------------------------------------------------------------------------------------
void CBaseCamera::Reset()
{
    XMVECTOR vDefaultEye = XMLoadFloat3( &m_vDefaultEye );
    XMVECTOR vDefaultLookAt = XMLoadFloat3( &m_vDefaultLookAt );

    SetViewParams( vDefaultEye, vDefaultLookAt );
}


//======================================================================================
// CFirstPersonCamera
//======================================================================================

CFirstPersonCamera::CFirstPersonCamera() :
    m_nActiveButtonMask( 0x07 ),
    m_bRotateWithoutButtonDown(false)
{
}


//--------------------------------------------------------------------------------------
// Update the view matrix based on user input & elapsed time
//--------------------------------------------------------------------------------------
void CFirstPersonCamera::FrameMove( _In_ float fElapsedTime )
{
    if( DXUTGetGlobalTimer()->IsStopped() )
    {
        if (DXUTGetFPS() == 0.0f)
            fElapsedTime = 0;
        else
            fElapsedTime = 1.0f / DXUTGetFPS();
    }

    if( IsKeyDown( m_aKeys[CAM_RESET] ) )
    {
        Reset();
    }

    // Get keyboard/mouse/gamepad input
    GetInput( m_bEnablePositionMovement, ( m_nActiveButtonMask & m_nCurrentButtonMask ) || m_bRotateWithoutButtonDown, true );

    //// Get the mouse movement (if any) if the mouse button are down
    //if( (m_nActiveButtonMask & m_nCurrentButtonMask) || m_bRotateWithoutButtonDown )
    //    UpdateMouseDelta( fElapsedTime );

    // Get amount of velocity based on the keyboard input and drag (if any)
    UpdateVelocity( fElapsedTime );

    // Simple euler method to calculate position delta
    XMVECTOR vVelocity = XMLoadFloat3( &m_vVelocity );
    XMVECTOR vPosDelta = vVelocity * fElapsedTime;

    // If rotating the camera 
    if( ( m_nActiveButtonMask & m_nCurrentButtonMask )
        || m_bRotateWithoutButtonDown
        || m_vGamePadRightThumb.x != 0
        || m_vGamePadRightThumb.z != 0 )
    {
        // Update the pitch & yaw angle based on mouse movement
        float fYawDelta = m_vRotVelocity.x;
        float fPitchDelta = m_vRotVelocity.y;

        // Invert pitch if requested
        if( m_bInvertPitch )
            fPitchDelta = -fPitchDelta;

        m_fCameraPitchAngle += fPitchDelta;
        m_fCameraYawAngle += fYawDelta;

        // Limit pitch to straight up or straight down
        m_fCameraPitchAngle = std::max( -XM_PI / 2.0f, m_fCameraPitchAngle );
        m_fCameraPitchAngle = std::min( +XM_PI / 2.0f, m_fCameraPitchAngle );
    }

    // Make a rotation matrix based on the camera's yaw & pitch
    XMMATRIX mCameraRot = XMMatrixRotationRollPitchYaw( m_fCameraPitchAngle, m_fCameraYawAngle, 0 );

    // Transform vectors based on camera's rotation matrix
    XMVECTOR vWorldUp = XMVector3TransformCoord( g_XMIdentityR1, mCameraRot );
    XMVECTOR vWorldAhead = XMVector3TransformCoord( g_XMIdentityR2, mCameraRot );

    // Transform the position delta by the camera's rotation 
    if( !m_bEnableYAxisMovement )
    {
        // If restricting Y movement, do not include pitch
        // when transforming position delta vector.
        mCameraRot = XMMatrixRotationRollPitchYaw( 0.0f, m_fCameraYawAngle, 0.0f );
    }
    XMVECTOR vPosDeltaWorld = XMVector3TransformCoord( vPosDelta, mCameraRot );

    // Move the eye position 
    XMVECTOR vEye = XMLoadFloat3( &m_vEye );
    vEye += vPosDeltaWorld;
    if( m_bClipToBoundary )
        vEye = ConstrainToBoundary( vEye );
    XMStoreFloat3( &m_vEye, vEye );

    // Update the lookAt position based on the eye position
    XMVECTOR vLookAt = vEye + vWorldAhead;
    XMStoreFloat3( &m_vLookAt, vLookAt );

    // Update the view matrix
    XMMATRIX mView = XMMatrixLookAtLH( vEye, vLookAt, vWorldUp );
    XMStoreFloat4x4( &m_mView, mView );

    XMMATRIX mCameraWorld = XMMatrixInverse( nullptr, mView );
    XMStoreFloat4x4( &m_mCameraWorld, mCameraWorld );
}


//--------------------------------------------------------------------------------------
// Enable or disable each of the mouse buttons for rotation drag.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CFirstPersonCamera::SetRotateButtons( bool bLeft, bool bMiddle, bool bRight, bool bRotateWithoutButtonDown )
{
    m_nActiveButtonMask = ( bLeft ? MOUSE_LEFT_BUTTON : 0 ) |
        ( bMiddle ? MOUSE_MIDDLE_BUTTON : 0 ) |
        ( bRight ? MOUSE_RIGHT_BUTTON : 0 );
    m_bRotateWithoutButtonDown = bRotateWithoutButtonDown;
}



//======================================================================================
// CModelViewerCamera
//======================================================================================

CModelViewerCamera::CModelViewerCamera() :
    m_nRotateModelButtonMask(MOUSE_LEFT_BUTTON),
    m_nZoomButtonMask(MOUSE_WHEEL),
    m_nRotateCameraButtonMask(MOUSE_RIGHT_BUTTON),
    m_bAttachCameraToModel(false),
    m_bLimitPitch(false),
    m_bDragSinceLastUpdate(true),
    m_fRadius(5.0f),
    m_fDefaultRadius(5.0f),
    m_fMinRadius(1.0f),
    m_fMaxRadius(FLT_MAX)
{
    XMMATRIX id = XMMatrixIdentity();

    XMStoreFloat4x4( &m_mWorld, id );
    XMStoreFloat4x4( &m_mModelRot, id );
    XMStoreFloat4x4( &m_mModelLastRot, id );
    XMStoreFloat4x4( &m_mCameraRotLast, id );
    m_vModelCenter = XMFLOAT3( 0, 0, 0 );

    m_bEnablePositionMovement = false;
}


//--------------------------------------------------------------------------------------
// Update the view matrix & the model's world matrix based 
//       on user input & elapsed time
//--------------------------------------------------------------------------------------
void CModelViewerCamera::FrameMove( _In_ float fElapsedTime )
{
    if( IsKeyDown( m_aKeys[CAM_RESET] ) )
        Reset();

    // If no dragged has happend since last time FrameMove is called,
    // and no camera key is held down, then no need to handle again.
    if( !m_bDragSinceLastUpdate && 0 == m_cKeysDown )
        return;

    m_bDragSinceLastUpdate = false;

    //// If no mouse button is held down, 
    //// Get the mouse movement (if any) if the mouse button are down
    //if( m_nCurrentButtonMask != 0 ) 
    //    UpdateMouseDelta( fElapsedTime );

    GetInput( m_bEnablePositionMovement, m_nCurrentButtonMask != 0, true );

    // Get amount of velocity based on the keyboard input and drag (if any)
    UpdateVelocity( fElapsedTime );

    // Simple euler method to calculate position delta
    XMVECTOR vPosDelta = XMLoadFloat3( &m_vVelocity ) * fElapsedTime;

    // Change the radius from the camera to the model based on wheel scrolling
    if( m_nMouseWheelDelta && m_nZoomButtonMask == MOUSE_WHEEL )
        m_fRadius -= m_nMouseWheelDelta * m_fRadius * 0.1f / 120.0f;
    m_fRadius = std::min( m_fMaxRadius, m_fRadius );
    m_fRadius = std::max( m_fMinRadius, m_fRadius );
    m_nMouseWheelDelta = 0;

    // Get the inverse of the arcball's rotation matrix
    XMMATRIX mCameraRot = XMMatrixInverse( nullptr, m_ViewArcBall.GetRotationMatrix() );

    // Transform vectors based on camera's rotation matrix
    XMVECTOR vWorldUp = XMVector3TransformCoord( g_XMIdentityR1, mCameraRot );
    XMVECTOR vWorldAhead = XMVector3TransformCoord( g_XMIdentityR2, mCameraRot );

    // Transform the position delta by the camera's rotation 
    XMVECTOR vPosDeltaWorld = XMVector3TransformCoord( vPosDelta, mCameraRot );

    // Move the lookAt position 
    XMVECTOR vLookAt = XMLoadFloat3( &m_vLookAt );
    vLookAt += vPosDeltaWorld;
    if( m_bClipToBoundary )
        vLookAt = ConstrainToBoundary( vLookAt );
    XMStoreFloat3( &m_vLookAt, vLookAt );

    // Update the eye point based on a radius away from the lookAt position
    XMVECTOR vEye = vLookAt - vWorldAhead * m_fRadius;
    XMStoreFloat3( &m_vEye, vEye );

    // Update the view matrix
    XMMATRIX mView = XMMatrixLookAtLH( vEye, vLookAt, vWorldUp );
    XMStoreFloat4x4( &m_mView, mView );

    XMMATRIX mInvView = XMMatrixInverse( nullptr, mView );
    mInvView.r[3] = XMVectorSelect( mInvView.r[3], g_XMZero, g_XMSelect1110 );

    XMMATRIX mModelLastRot = XMLoadFloat4x4( &m_mModelLastRot );
    XMMATRIX mModelLastRotInv = XMMatrixInverse( nullptr, mModelLastRot );

    // Accumulate the delta of the arcball's rotation in view space.
    // Note that per-frame delta rotations could be problematic over long periods of time.
    XMMATRIX mModelRot0 = m_WorldArcBall.GetRotationMatrix();
    XMMATRIX mModelRot = XMLoadFloat4x4( &m_mModelRot );
    mModelRot *= mView * mModelLastRotInv * mModelRot0 * mInvView;

    if( m_ViewArcBall.IsBeingDragged() && m_bAttachCameraToModel && !IsKeyDown( m_aKeys[CAM_CONTROLDOWN] ) )
    {
        // Attach camera to model by inverse of the model rotation
        XMMATRIX mCameraRotLast = XMLoadFloat4x4( &m_mCameraRotLast );
        XMMATRIX mCameraLastRotInv = XMMatrixInverse( nullptr, mCameraRotLast );
        XMMATRIX mCameraRotDelta = mCameraLastRotInv * mCameraRot; // local to world matrix
        mModelRot *= mCameraRotDelta;
    }

    XMStoreFloat4x4( &m_mModelLastRot, mModelRot0 );
    XMStoreFloat4x4( &m_mCameraRotLast, mCameraRot );

    // Since we're accumulating delta rotations, we need to orthonormalize 
    // the matrix to prevent eventual matrix skew
    XMVECTOR xBasis = XMVector3Normalize( mModelRot.r[0] );
    XMVECTOR yBasis = XMVector3Cross( mModelRot.r[2], xBasis );
    yBasis = XMVector3Normalize( yBasis );
    XMVECTOR zBasis = XMVector3Cross( xBasis, yBasis );

    mModelRot.r[0] = XMVectorSelect( mModelRot.r[0], xBasis, g_XMSelect1110 );
    mModelRot.r[1] = XMVectorSelect( mModelRot.r[1], yBasis, g_XMSelect1110 );
    mModelRot.r[2] = XMVectorSelect( mModelRot.r[2], zBasis, g_XMSelect1110 );

    // Translate the rotation matrix to the same position as the lookAt position
    mModelRot.r[3] = XMVectorSelect( mModelRot.r[3], vLookAt, g_XMSelect1110 );
    
    XMStoreFloat4x4( &m_mModelRot, mModelRot );

    // Translate world matrix so its at the center of the model
    XMMATRIX mTrans = XMMatrixTranslation( -m_vModelCenter.x, -m_vModelCenter.y, -m_vModelCenter.z );
    XMMATRIX mWorld = mTrans * mModelRot;
    XMStoreFloat4x4( &m_mWorld, mWorld );
}


//--------------------------------------------------------------------------------------
void CModelViewerCamera::SetDragRect( _In_ const RECT& rc )
{
    CBaseCamera::SetDragRect( rc );

    m_WorldArcBall.SetOffset( rc.left, rc.top );
    m_ViewArcBall.SetOffset( rc.left, rc.top );

    SetWindow( rc.right - rc.left, rc.bottom - rc.top );
}


//--------------------------------------------------------------------------------------
// Reset the camera's position back to the default
//--------------------------------------------------------------------------------------
void CModelViewerCamera::Reset()
{
    CBaseCamera::Reset();

    XMMATRIX id = XMMatrixIdentity();
    XMStoreFloat4x4( &m_mWorld, id );
    XMStoreFloat4x4( &m_mModelRot, id );
    XMStoreFloat4x4( &m_mModelLastRot, id );
    XMStoreFloat4x4( &m_mCameraRotLast, id );

    m_fRadius = m_fDefaultRadius;
    m_WorldArcBall.Reset();
    m_ViewArcBall.Reset();
}


//--------------------------------------------------------------------------------------
// Override for setting the view parameters
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CModelViewerCamera::SetViewParams( FXMVECTOR vEyePt, FXMVECTOR vLookatPt )
{
    CBaseCamera::SetViewParams( vEyePt, vLookatPt );

    // Propogate changes to the member arcball
    XMMATRIX mRotation = XMMatrixLookAtLH( vEyePt, vLookatPt, g_XMIdentityR1 );
    XMVECTOR quat = XMQuaternionRotationMatrix( mRotation );
    m_ViewArcBall.SetQuatNow( quat );

    // Set the radius according to the distance
    XMVECTOR vEyeToPoint = XMVectorSubtract( vLookatPt, vEyePt );
    float len = XMVectorGetX( XMVector3Length( vEyeToPoint ) );
    SetRadius( len );

    // View information changed. FrameMove should be called.
    m_bDragSinceLastUpdate = true;
}


//--------------------------------------------------------------------------------------
// Call this from your message proc so this class can handle window messages
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
LRESULT CModelViewerCamera::HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CBaseCamera::HandleMessages( hWnd, uMsg, wParam, lParam );

    if( ( ( uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK ) && m_nRotateModelButtonMask & MOUSE_LEFT_BUTTON ) ||
        ( ( uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONDBLCLK ) && m_nRotateModelButtonMask & MOUSE_MIDDLE_BUTTON ) ||
        ( ( uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONDBLCLK ) && m_nRotateModelButtonMask & MOUSE_RIGHT_BUTTON ) )
    {
        int iMouseX = ( short )LOWORD( lParam );
        int iMouseY = ( short )HIWORD( lParam );
        m_WorldArcBall.OnBegin( iMouseX, iMouseY );
    }

    if( ( ( uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK ) && m_nRotateCameraButtonMask & MOUSE_LEFT_BUTTON ) ||
        ( ( uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONDBLCLK ) &&
          m_nRotateCameraButtonMask & MOUSE_MIDDLE_BUTTON ) ||
        ( ( uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONDBLCLK ) && m_nRotateCameraButtonMask & MOUSE_RIGHT_BUTTON ) )
    {
        int iMouseX = ( short )LOWORD( lParam );
        int iMouseY = ( short )HIWORD( lParam );
        m_ViewArcBall.OnBegin( iMouseX, iMouseY );
    }

    if( uMsg == WM_MOUSEMOVE )
    {
        int iMouseX = ( short )LOWORD( lParam );
        int iMouseY = ( short )HIWORD( lParam );
        m_WorldArcBall.OnMove( iMouseX, iMouseY );
        m_ViewArcBall.OnMove( iMouseX, iMouseY );
    }

    if( ( uMsg == WM_LBUTTONUP && m_nRotateModelButtonMask & MOUSE_LEFT_BUTTON ) ||
        ( uMsg == WM_MBUTTONUP && m_nRotateModelButtonMask & MOUSE_MIDDLE_BUTTON ) ||
        ( uMsg == WM_RBUTTONUP && m_nRotateModelButtonMask & MOUSE_RIGHT_BUTTON ) )
    {
        m_WorldArcBall.OnEnd();
    }

    if( ( uMsg == WM_LBUTTONUP && m_nRotateCameraButtonMask & MOUSE_LEFT_BUTTON ) ||
        ( uMsg == WM_MBUTTONUP && m_nRotateCameraButtonMask & MOUSE_MIDDLE_BUTTON ) ||
        ( uMsg == WM_RBUTTONUP && m_nRotateCameraButtonMask & MOUSE_RIGHT_BUTTON ) )
    {
        m_ViewArcBall.OnEnd();
    }

    if( uMsg == WM_CAPTURECHANGED )
    {
        if( ( HWND )lParam != hWnd )
        {
            if( ( m_nRotateModelButtonMask & MOUSE_LEFT_BUTTON ) ||
                ( m_nRotateModelButtonMask & MOUSE_MIDDLE_BUTTON ) ||
                ( m_nRotateModelButtonMask & MOUSE_RIGHT_BUTTON ) )
            {
                m_WorldArcBall.OnEnd();
            }

            if( ( m_nRotateCameraButtonMask & MOUSE_LEFT_BUTTON ) ||
                ( m_nRotateCameraButtonMask & MOUSE_MIDDLE_BUTTON ) ||
                ( m_nRotateCameraButtonMask & MOUSE_RIGHT_BUTTON ) )
            {
                m_ViewArcBall.OnEnd();
            }
        }
    }

    if( uMsg == WM_LBUTTONDOWN ||
        uMsg == WM_LBUTTONDBLCLK ||
        uMsg == WM_MBUTTONDOWN ||
        uMsg == WM_MBUTTONDBLCLK ||
        uMsg == WM_RBUTTONDOWN ||
        uMsg == WM_RBUTTONDBLCLK ||
        uMsg == WM_LBUTTONUP ||
        uMsg == WM_MBUTTONUP ||
        uMsg == WM_RBUTTONUP ||
        uMsg == WM_MOUSEWHEEL ||
        uMsg == WM_MOUSEMOVE )
    {
        m_bDragSinceLastUpdate = true;
    }

    return FALSE;
}


//======================================================================================
// CDXUTDirectionWidget
//======================================================================================

CDXUTDirectionWidget::CDXUTDirectionWidget() :
    m_fRadius(1.0f),
    m_nRotateMask(MOUSE_RIGHT_BUTTON)
{
    m_vDefaultDir = XMFLOAT3( 0, 1, 0 );
    m_vCurrentDir = m_vDefaultDir;

    XMMATRIX id = XMMatrixIdentity();

    XMStoreFloat4x4( &m_mView, id );
    XMStoreFloat4x4( &m_mRot, id );
    XMStoreFloat4x4( &m_mRotSnapshot, id );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
LRESULT CDXUTDirectionWidget::HandleMessages( HWND hWnd, UINT uMsg,
                                              WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(wParam);

    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            {
                if( ( ( m_nRotateMask & MOUSE_LEFT_BUTTON ) != 0 && uMsg == WM_LBUTTONDOWN ) ||
                    ( ( m_nRotateMask & MOUSE_MIDDLE_BUTTON ) != 0 && uMsg == WM_MBUTTONDOWN ) ||
                    ( ( m_nRotateMask & MOUSE_RIGHT_BUTTON ) != 0 && uMsg == WM_RBUTTONDOWN ) )
                {
                    int iMouseX = ( int )( short )LOWORD( lParam );
                    int iMouseY = ( int )( short )HIWORD( lParam );
                    m_ArcBall.OnBegin( iMouseX, iMouseY );
                    SetCapture( hWnd );
                }
                return TRUE;
            }

        case WM_MOUSEMOVE:
        {
            if( m_ArcBall.IsBeingDragged() )
            {
                int iMouseX = ( int )( short )LOWORD( lParam );
                int iMouseY = ( int )( short )HIWORD( lParam );
                m_ArcBall.OnMove( iMouseX, iMouseY );
                UpdateLightDir();
            }
            return TRUE;
        }

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            {
                if( ( ( m_nRotateMask & MOUSE_LEFT_BUTTON ) != 0 && uMsg == WM_LBUTTONUP ) ||
                    ( ( m_nRotateMask & MOUSE_MIDDLE_BUTTON ) != 0 && uMsg == WM_MBUTTONUP ) ||
                    ( ( m_nRotateMask & MOUSE_RIGHT_BUTTON ) != 0 && uMsg == WM_RBUTTONUP ) )
                {
                    m_ArcBall.OnEnd();
                    ReleaseCapture();
                }

                UpdateLightDir();
                return TRUE;
            }

        case WM_CAPTURECHANGED:
        {
            if( ( HWND )lParam != hWnd )
            {
                if( ( m_nRotateMask & MOUSE_LEFT_BUTTON ) ||
                    ( m_nRotateMask & MOUSE_MIDDLE_BUTTON ) ||
                    ( m_nRotateMask & MOUSE_RIGHT_BUTTON ) )
                {
                    m_ArcBall.OnEnd();
                    ReleaseCapture();
                }
            }
            return TRUE;
        }
    }

    return 0;
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTDirectionWidget::UpdateLightDir()
{
    XMMATRIX mView = XMLoadFloat4x4( &m_mView );

    XMMATRIX mInvView = XMMatrixInverse( nullptr, mView );
    mInvView.r[3] = XMVectorSelect( mInvView.r[3], g_XMZero, g_XMSelect1110 );

    XMMATRIX mRotSnapshot = XMLoadFloat4x4( &m_mRotSnapshot );
    XMMATRIX mLastRotInv = XMMatrixInverse( nullptr, mRotSnapshot );

    XMMATRIX mRot0 = m_ArcBall.GetRotationMatrix();
    XMStoreFloat4x4( &m_mRotSnapshot, mRot0 );

    // Accumulate the delta of the arcball's rotation in view space.
    // Note that per-frame delta rotations could be problematic over long periods of time.
    XMMATRIX mRot = XMLoadFloat4x4( &m_mRot );
    mRot *= mView * mLastRotInv * mRot0 * mInvView;

    // Since we're accumulating delta rotations, we need to orthonormalize 
    // the matrix to prevent eventual matrix skew
    XMVECTOR xBasis = XMVector3Normalize( mRot.r[0] );
    XMVECTOR yBasis = XMVector3Cross( mRot.r[2], xBasis );
    yBasis = XMVector3Normalize( yBasis );
    XMVECTOR zBasis = XMVector3Cross( xBasis, yBasis );
    mRot.r[0] = XMVectorSelect( mRot.r[0], xBasis, g_XMSelect1110 );
    mRot.r[1] = XMVectorSelect( mRot.r[1], yBasis, g_XMSelect1110 );
    mRot.r[2] = XMVectorSelect( mRot.r[2], zBasis, g_XMSelect1110 );
    XMStoreFloat4x4( &m_mRot, mRot );

    // Transform the default direction vector by the light's rotation matrix
    XMVECTOR vDefaultDir = XMLoadFloat3( &m_vDefaultDir );
    XMVECTOR vCurrentDir = XMVector3TransformNormal( vDefaultDir, mRot );
    XMStoreFloat3( &m_vCurrentDir, vCurrentDir );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDirectionWidget::OnRender( FXMVECTOR color, CXMMATRIX mView, CXMMATRIX mProj, FXMVECTOR vEyePt )
{
    UNREFERENCED_PARAMETER(color);
    UNREFERENCED_PARAMETER(mView);
    UNREFERENCED_PARAMETER(mProj);
    UNREFERENCED_PARAMETER(vEyePt);
    // TODO - 
    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDirectionWidget::StaticOnD3D11CreateDevice( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext )
{
    UNREFERENCED_PARAMETER(pd3dDevice);
    UNREFERENCED_PARAMETER(pd3dImmediateContext);
    // TODO -
    return S_OK;
}


//--------------------------------------------------------------------------------------
void CDXUTDirectionWidget::StaticOnD3D11DestroyDevice()
{
    // TODO -
}

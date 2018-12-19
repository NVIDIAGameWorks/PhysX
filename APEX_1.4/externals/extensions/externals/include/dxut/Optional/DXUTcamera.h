//--------------------------------------------------------------------------------------
// File: Camera.h
//
// Helper functions for Direct3D programming.
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
#pragma once

//--------------------------------------------------------------------------------------
class CD3DArcBall
{
public:
    CD3DArcBall();

    // Functions to change behavior
    void Reset();
    void SetTranslationRadius( _In_ float fRadiusTranslation )
    {
        m_fRadiusTranslation = fRadiusTranslation;
    }
    void SetWindow( _In_ INT nWidth, _In_ INT nHeight, _In_ float fRadius = 0.9f )
    {
        m_nWidth = nWidth;
        m_nHeight = nHeight;
        m_fRadius = fRadius;
        m_vCenter.x = float(m_nWidth) / 2.0f;
        m_vCenter.y = float(m_nHeight) / 2.0f;
    }
    void SetOffset( _In_ INT nX, _In_ INT nY ) { m_Offset.x = nX; m_Offset.y = nY; }

    // Call these from client and use GetRotationMatrix() to read new rotation matrix
    void OnBegin( _In_ int nX, _In_ int nY );   // start the rotation (pass current mouse position)
    void OnMove( _In_ int nX, _In_ int nY );    // continue the rotation (pass current mouse position)
    void OnEnd();                               // end the rotation 

    // Or call this to automatically handle left, middle, right buttons
    LRESULT HandleMessages( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );

    // Functions to get/set state
    DirectX::XMMATRIX GetRotationMatrix() const
    {
        using namespace DirectX;
        XMVECTOR q = XMLoadFloat4( &m_qNow );
        return DirectX::XMMatrixRotationQuaternion( q );
    }
    DirectX::XMMATRIX GetTranslationMatrix() const { return DirectX::XMLoadFloat4x4( &m_mTranslation ); }
    DirectX::XMMATRIX GetTranslationDeltaMatrix() const { return DirectX::XMLoadFloat4x4( &m_mTranslationDelta ); }
    bool IsBeingDragged() const { return m_bDrag; }
    DirectX::XMVECTOR GetQuatNow() const { return DirectX::XMLoadFloat4( &m_qNow ); }
    void SetQuatNow( _In_ DirectX::FXMVECTOR& q ) { DirectX::XMStoreFloat4( &m_qNow, q ); }

    static DirectX::XMVECTOR QuatFromBallPoints( _In_ DirectX::FXMVECTOR vFrom, _In_ DirectX::FXMVECTOR vTo )
    {
        using namespace DirectX;

        XMVECTOR dot = XMVector3Dot( vFrom, vTo );
        XMVECTOR vPart = XMVector3Cross( vFrom, vTo );
        return XMVectorSelect( dot, vPart, g_XMSelect1110 );
    }

protected:
    DirectX::XMFLOAT4X4 m_mRotation;        // Matrix for arc ball's orientation
    DirectX::XMFLOAT4X4 m_mTranslation;     // Matrix for arc ball's position
    DirectX::XMFLOAT4X4 m_mTranslationDelta;// Matrix for arc ball's position

    POINT m_Offset;                         // window offset, or upper-left corner of window
    INT m_nWidth;                           // arc ball's window width
    INT m_nHeight;                          // arc ball's window height
    DirectX::XMFLOAT2 m_vCenter;            // center of arc ball 
    float m_fRadius;                        // arc ball's radius in screen coords
    float m_fRadiusTranslation;             // arc ball's radius for translating the target

    DirectX::XMFLOAT4 m_qDown;              // Quaternion before button down
    DirectX::XMFLOAT4 m_qNow;               // Composite quaternion for current drag
    bool m_bDrag;                           // Whether user is dragging arc ball

    POINT m_ptLastMouse;                    // position of last mouse point
    DirectX::XMFLOAT3 m_vDownPt;            // starting point of rotation arc
    DirectX::XMFLOAT3 m_vCurrentPt;         // current point of rotation arc

    DirectX::XMVECTOR ScreenToVector( _In_ float fScreenPtX, _In_ float fScreenPtY )
    {
        // Scale to screen
        float x = -( fScreenPtX - m_Offset.x - m_nWidth / 2 ) / ( m_fRadius * m_nWidth / 2 );
        float y = ( fScreenPtY - m_Offset.y - m_nHeight / 2 ) / ( m_fRadius * m_nHeight / 2 );

        float z = 0.0f;
        float mag = x * x + y * y;

        if( mag > 1.0f )
        {
            float scale = 1.0f / sqrtf( mag );
            x *= scale;
            y *= scale;
        }
        else
            z = sqrtf( 1.0f - mag );

        return DirectX::XMVectorSet( x, y, z, 0 );
    }
};


//--------------------------------------------------------------------------------------
// used by CCamera to map WM_KEYDOWN keys
//--------------------------------------------------------------------------------------
enum D3DUtil_CameraKeys
{
    CAM_STRAFE_LEFT = 0,
    CAM_STRAFE_RIGHT,
    CAM_MOVE_FORWARD,
    CAM_MOVE_BACKWARD,
    CAM_MOVE_UP,
    CAM_MOVE_DOWN,
    CAM_RESET,
    CAM_CONTROLDOWN,
    CAM_MAX_KEYS,
    CAM_UNKNOWN     = 0xFF
};

#define KEY_WAS_DOWN_MASK 0x80
#define KEY_IS_DOWN_MASK  0x01

#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_MIDDLE_BUTTON 0x02
#define MOUSE_RIGHT_BUTTON  0x04
#define MOUSE_WHEEL         0x08


//--------------------------------------------------------------------------------------
// Simple base camera class that moves and rotates.  The base class
//       records mouse and keyboard input for use by a derived class, and 
//       keeps common state.
//--------------------------------------------------------------------------------------
class CBaseCamera
{
public:
    CBaseCamera();

    // Call these from client and use Get*Matrix() to read new matrices
    virtual LRESULT HandleMessages( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );
    virtual void FrameMove( _In_ float fElapsedTime ) = 0;

    // Functions to change camera matrices
    virtual void Reset();
    virtual void SetViewParams( _In_ DirectX::FXMVECTOR vEyePt, _In_ DirectX::FXMVECTOR vLookatPt );
    virtual void SetProjParams( _In_ float fFOV, _In_ float fAspect, _In_ float fNearPlane, _In_ float fFarPlane );

    // Functions to change behavior
    virtual void SetDragRect( _In_ const RECT& rc ) { m_rcDrag = rc; }
    void SetInvertPitch( _In_ bool bInvertPitch ) { m_bInvertPitch = bInvertPitch; }
    void SetDrag( _In_ bool bMovementDrag, _In_ float fTotalDragTimeToZero = 0.25f )
    {
        m_bMovementDrag = bMovementDrag;
        m_fTotalDragTimeToZero = fTotalDragTimeToZero;
    }
    void SetEnableYAxisMovement( _In_ bool bEnableYAxisMovement ) { m_bEnableYAxisMovement = bEnableYAxisMovement; }
    void SetEnablePositionMovement( _In_ bool bEnablePositionMovement ) { m_bEnablePositionMovement = bEnablePositionMovement; }
    void SetClipToBoundary( _In_ bool bClipToBoundary, _In_opt_ DirectX::XMFLOAT3* pvMinBoundary, _In_opt_ DirectX::XMFLOAT3* pvMaxBoundary )
    {
        m_bClipToBoundary = bClipToBoundary;
        if( pvMinBoundary ) m_vMinBoundary = *pvMinBoundary;
        if( pvMaxBoundary ) m_vMaxBoundary = *pvMaxBoundary;
    }
    void SetScalers( _In_ float fRotationScaler = 0.01f, _In_ float fMoveScaler = 5.0f )
    {
        m_fRotationScaler = fRotationScaler;
        m_fMoveScaler = fMoveScaler;
    }
    void SetNumberOfFramesToSmoothMouseData( _In_ int nFrames ) { if( nFrames > 0 ) m_fFramesToSmoothMouseData = ( float )nFrames; }
    void SetResetCursorAfterMove( _In_ bool bResetCursorAfterMove ) { m_bResetCursorAfterMove = bResetCursorAfterMove; }

    // Functions to get state
    DirectX::XMMATRIX GetViewMatrix() const { return DirectX::XMLoadFloat4x4( &m_mView ); }
    DirectX::XMMATRIX GetProjMatrix() const { return DirectX::XMLoadFloat4x4( &m_mProj ); }
    DirectX::XMVECTOR GetEyePt() const { return DirectX::XMLoadFloat3( &m_vEye ); }
    DirectX::XMVECTOR GetLookAtPt() const { return DirectX::XMLoadFloat3( &m_vLookAt ); }
    float GetNearClip() const { return m_fNearPlane; }
    float GetFarClip() const { return m_fFarPlane; }

    bool IsBeingDragged() const { return ( m_bMouseLButtonDown || m_bMouseMButtonDown || m_bMouseRButtonDown ); }
    bool IsMouseLButtonDown() const { return m_bMouseLButtonDown; }
    bool IsMouseMButtonDown() const { return m_bMouseMButtonDown; }
    bool sMouseRButtonDown() const { return m_bMouseRButtonDown; }

protected:
    // Functions to map a WM_KEYDOWN key to a D3DUtil_CameraKeys enum
    virtual D3DUtil_CameraKeys MapKey( _In_ UINT nKey );

    bool IsKeyDown( _In_ BYTE key ) const { return( ( key & KEY_IS_DOWN_MASK ) == KEY_IS_DOWN_MASK ); }
    bool WasKeyDown( _In_ BYTE key ) const { return( ( key & KEY_WAS_DOWN_MASK ) == KEY_WAS_DOWN_MASK ); }

    DirectX::XMVECTOR ConstrainToBoundary( _In_ DirectX::FXMVECTOR v )
    {
        using namespace DirectX;

        XMVECTOR vMin = XMLoadFloat3( &m_vMinBoundary );
        XMVECTOR vMax = XMLoadFloat3( &m_vMaxBoundary );

        // Constrain vector to a bounding box 
        return XMVectorClamp( v, vMin, vMax );
    }

    void UpdateMouseDelta();
    void UpdateVelocity( _In_ float fElapsedTime );
    void GetInput( _In_ bool bGetKeyboardInput, _In_ bool bGetMouseInput, _In_ bool bGetGamepadInput );

    DirectX::XMFLOAT4X4 m_mView;                    // View matrix 
    DirectX::XMFLOAT4X4 m_mProj;                    // Projection matrix

    DXUT_GAMEPAD m_GamePad[DXUT_MAX_CONTROLLERS];  // XInput controller state
    DirectX::XMFLOAT3 m_vGamePadLeftThumb;
    DirectX::XMFLOAT3 m_vGamePadRightThumb;
    double m_GamePadLastActive[DXUT_MAX_CONTROLLERS];

    int m_cKeysDown;                        // Number of camera keys that are down.
    BYTE m_aKeys[CAM_MAX_KEYS];             // State of input - KEY_WAS_DOWN_MASK|KEY_IS_DOWN_MASK
    DirectX::XMFLOAT3 m_vKeyboardDirection; // Direction vector of keyboard input
    POINT m_ptLastMousePosition;            // Last absolute position of mouse cursor
    int m_nCurrentButtonMask;               // mask of which buttons are down
    int m_nMouseWheelDelta;                 // Amount of middle wheel scroll (+/-) 
    DirectX::XMFLOAT2 m_vMouseDelta;        // Mouse relative delta smoothed over a few frames
    float m_fFramesToSmoothMouseData;       // Number of frames to smooth mouse data over
    DirectX::XMFLOAT3 m_vDefaultEye;        // Default camera eye position
    DirectX::XMFLOAT3 m_vDefaultLookAt;     // Default LookAt position
    DirectX::XMFLOAT3 m_vEye;               // Camera eye position
    DirectX::XMFLOAT3 m_vLookAt;            // LookAt position
    float m_fCameraYawAngle;                // Yaw angle of camera
    float m_fCameraPitchAngle;              // Pitch angle of camera

    RECT m_rcDrag;                          // Rectangle within which a drag can be initiated.
    DirectX::XMFLOAT3 m_vVelocity;          // Velocity of camera
    DirectX::XMFLOAT3 m_vVelocityDrag;      // Velocity drag force
    float m_fDragTimer;                     // Countdown timer to apply drag
    float m_fTotalDragTimeToZero;           // Time it takes for velocity to go from full to 0
    DirectX::XMFLOAT2 m_vRotVelocity;       // Velocity of camera

    float m_fFOV;                           // Field of view
    float m_fAspect;                        // Aspect ratio
    float m_fNearPlane;                     // Near plane
    float m_fFarPlane;                      // Far plane

    float m_fRotationScaler;                // Scaler for rotation
    float m_fMoveScaler;                    // Scaler for movement

    bool m_bMouseLButtonDown;               // True if left button is down 
    bool m_bMouseMButtonDown;               // True if middle button is down 
    bool m_bMouseRButtonDown;               // True if right button is down 
    bool m_bMovementDrag;                   // If true, then camera movement will slow to a stop otherwise movement is instant
    bool m_bInvertPitch;                    // Invert the pitch axis
    bool m_bEnablePositionMovement;         // If true, then the user can translate the camera/model 
    bool m_bEnableYAxisMovement;            // If true, then camera can move in the y-axis
    bool m_bClipToBoundary;                 // If true, then the camera will be clipped to the boundary
    bool m_bResetCursorAfterMove;           // If true, the class will reset the cursor position so that the cursor always has space to move 

    DirectX::XMFLOAT3 m_vMinBoundary;       // Min point in clip boundary
    DirectX::XMFLOAT3 m_vMaxBoundary;       // Max point in clip boundary
};


//--------------------------------------------------------------------------------------
// Simple first person camera class that moves and rotates.
//       It allows yaw and pitch but not roll.  It uses WM_KEYDOWN and 
//       GetCursorPos() to respond to keyboard and mouse input and updates the 
//       view matrix based on input.  
//--------------------------------------------------------------------------------------
class CFirstPersonCamera : public CBaseCamera
{
public:
    CFirstPersonCamera();

    // Call these from client and use Get*Matrix() to read new matrices
    virtual void FrameMove( _In_ float fElapsedTime ) override;

    // Functions to change behavior
    void SetRotateButtons( _In_ bool bLeft, _In_ bool bMiddle, _In_ bool bRight, _In_ bool bRotateWithoutButtonDown = false );

    // Functions to get state
    DirectX::XMMATRIX GetWorldMatrix() const { return DirectX::XMLoadFloat4x4( &m_mCameraWorld ); }

    DirectX::XMVECTOR GetWorldRight() const { return DirectX::XMLoadFloat3( reinterpret_cast<const DirectX::XMFLOAT3*>( &m_mCameraWorld._11 ) ); }
    DirectX::XMVECTOR GetWorldUp() const { return DirectX::XMLoadFloat3( reinterpret_cast<const DirectX::XMFLOAT3*>( &m_mCameraWorld._21 ) ); }
    DirectX::XMVECTOR GetWorldAhead() const { return DirectX::XMLoadFloat3( reinterpret_cast<const DirectX::XMFLOAT3*>( &m_mCameraWorld._31 ) ); }
    DirectX::XMVECTOR GetEyePt() const { return DirectX::XMLoadFloat3( reinterpret_cast<const DirectX::XMFLOAT3*>( &m_mCameraWorld._41 ) ); }

protected:
    DirectX::XMFLOAT4X4 m_mCameraWorld; // World matrix of the camera (inverse of the view matrix)

    int m_nActiveButtonMask;            // Mask to determine which button to enable for rotation
    bool m_bRotateWithoutButtonDown;
};


//--------------------------------------------------------------------------------------
// Simple model viewing camera class that rotates around the object.
//--------------------------------------------------------------------------------------
class CModelViewerCamera : public CBaseCamera
{
public:
    CModelViewerCamera();

    // Call these from client and use Get*Matrix() to read new matrices
    virtual LRESULT HandleMessages( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual void FrameMove( _In_ float fElapsedTime ) override;

    // Functions to change behavior
    virtual void SetDragRect( _In_ const RECT& rc ) override;
    virtual void Reset() override;
    virtual void SetViewParams( _In_ DirectX::FXMVECTOR pvEyePt, _In_ DirectX::FXMVECTOR pvLookatPt ) override;
    void SetButtonMasks( _In_ int nRotateModelButtonMask = MOUSE_LEFT_BUTTON, _In_ int nZoomButtonMask = MOUSE_WHEEL,
                         _In_ int nRotateCameraButtonMask = MOUSE_RIGHT_BUTTON )
    {
        m_nRotateModelButtonMask = nRotateModelButtonMask, m_nZoomButtonMask = nZoomButtonMask;
        m_nRotateCameraButtonMask = nRotateCameraButtonMask;
    }
    void SetAttachCameraToModel( _In_ bool bEnable = false ) { m_bAttachCameraToModel = bEnable; }
    void SetWindow( _In_ int nWidth, _In_ int nHeight, _In_ float fArcballRadius=0.9f )
    {
        m_WorldArcBall.SetWindow( nWidth, nHeight, fArcballRadius );
        m_ViewArcBall.SetWindow( nWidth, nHeight, fArcballRadius );
    }
    void SetRadius( _In_ float fDefaultRadius=5.0f, _In_ float fMinRadius=1.0f, _In_ float fMaxRadius=FLT_MAX )
    {
        m_fDefaultRadius = m_fRadius = fDefaultRadius; m_fMinRadius = fMinRadius; m_fMaxRadius = fMaxRadius;
        m_bDragSinceLastUpdate = true;
    }
    void SetModelCenter( _In_ const DirectX::XMFLOAT3& vModelCenter ) { m_vModelCenter = vModelCenter; }
    void SetLimitPitch( _In_ bool bLimitPitch ) { m_bLimitPitch = bLimitPitch; }
    void SetViewQuat( _In_ DirectX::FXMVECTOR q )
    {
        m_ViewArcBall.SetQuatNow( q );
        m_bDragSinceLastUpdate = true;
    }
    void SetWorldQuat( _In_ DirectX::FXMVECTOR q )
    {
        m_WorldArcBall.SetQuatNow( q );
        m_bDragSinceLastUpdate = true;
    }

    // Functions to get state
    DirectX::XMMATRIX GetWorldMatrix() const { return DirectX::XMLoadFloat4x4( &m_mWorld ); }
    void SetWorldMatrix( _In_ DirectX::CXMMATRIX mWorld )
    {
        XMStoreFloat4x4( &m_mWorld, mWorld );
        m_bDragSinceLastUpdate = true;
    }

protected:
    CD3DArcBall m_WorldArcBall;
    CD3DArcBall m_ViewArcBall;
    DirectX::XMFLOAT3 m_vModelCenter;
    DirectX::XMFLOAT4X4 m_mModelLastRot;     // Last arcball rotation matrix for model 
    DirectX::XMFLOAT4X4 m_mModelRot;         // Rotation matrix of model
    DirectX::XMFLOAT4X4 m_mWorld;            // World matrix of model

    int m_nRotateModelButtonMask;
    int m_nZoomButtonMask;
    int m_nRotateCameraButtonMask;

    bool m_bAttachCameraToModel;
    bool m_bLimitPitch;
    bool m_bDragSinceLastUpdate;            // True if mouse drag has happened since last time FrameMove is called.
    float m_fRadius;                        // Distance from the camera to model 
    float m_fDefaultRadius;                 // Distance from the camera to model 
    float m_fMinRadius;                     // Min radius
    float m_fMaxRadius;                     // Max radius

    DirectX::XMFLOAT4X4 m_mCameraRotLast;
};


//--------------------------------------------------------------------------------------
// Manages the mesh, direction, mouse events of a directional arrow that 
// rotates around a radius controlled by an arcball 
//--------------------------------------------------------------------------------------
class CDXUTDirectionWidget
{
public:
    CDXUTDirectionWidget();

    LRESULT HandleMessages( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );

    HRESULT OnRender( _In_ DirectX::FXMVECTOR color, _In_ DirectX::CXMMATRIX pmView, _In_ DirectX::CXMMATRIX pmProj, _In_ DirectX::FXMVECTOR vEyePt );

    DirectX::XMVECTOR GetLightDirection() const { return DirectX::XMLoadFloat3( &m_vCurrentDir ); }
    void SetLightDirection( _In_ DirectX::FXMVECTOR vDir )
    {
        DirectX::XMStoreFloat3( &m_vCurrentDir, vDir );
        m_vDefaultDir = m_vCurrentDir;
    }
    void SetLightDirection( _In_ DirectX::XMFLOAT3 vDir )
    {
        m_vDefaultDir = m_vCurrentDir = vDir;
    }
    void SetButtonMask( _In_ int nRotate = MOUSE_RIGHT_BUTTON ) { m_nRotateMask = nRotate; }

    float GetRadius() const { return m_fRadius; }
    void SetRadius( _In_ float fRadius ) { m_fRadius = fRadius; }

    bool IsBeingDragged() { return m_ArcBall.IsBeingDragged(); }

    static HRESULT WINAPI StaticOnD3D11CreateDevice( _In_ ID3D11Device* pd3dDevice, _In_ ID3D11DeviceContext* pd3dImmediateContext );
    static void WINAPI StaticOnD3D11DestroyDevice();

protected:
    HRESULT UpdateLightDir();

    // TODO - need support for Direct3D 11 widget

    DirectX::XMFLOAT4X4 m_mRot;
    DirectX::XMFLOAT4X4 m_mRotSnapshot;
    float m_fRadius;
    int m_nRotateMask;
    CD3DArcBall m_ArcBall;
    DirectX::XMFLOAT3 m_vDefaultDir;
    DirectX::XMFLOAT3 m_vCurrentDir;
    DirectX::XMFLOAT4X4 m_mView;
};

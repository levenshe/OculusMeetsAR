#ifndef APP_H
#define APP_H

#include "OGRE/Ogre.h"
#include "OIS/OIS.h"
#include "ARLib/Oculus/Rift.h"
#include "ARLib/Tracking/TrackingManager.h"
#include "ARLib/ARLibOgre.h"
#include "WebcamScene.h"
#include "OgreBullet/Dynamics/OgreBulletDynamicsRigidBody.h"
#include "videoplayer.hpp"


class WebcamApp : public Ogre::FrameListener, public OIS::KeyListener, public OIS::MouseListener
{
	public:
		WebcamApp(bool showDebugWindow);
		~WebcamApp();

		void quit();

		bool keyPressed(const OIS::KeyEvent& e);
		bool keyReleased(const OIS::KeyEvent& e);
		bool mouseMoved(const OIS::MouseEvent& e);
		bool mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID id);
		bool mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID id);

		bool frameRenderingQueued(const Ogre::FrameEvent& evt);

		bool update();

	private:
		webcam::VideoPlayer *mVideoPlayerLeft;
		webcam::VideoPlayer *mVideoPlayerRight;
		void initOgre(bool showDebugWindow);
		void quitOgre();
        void initBullet(bool enableDebugDrawing);
        void quitBullet();
		void initOIS();
		void quitOIS();
		void initRift();
		void quitRift();
		void initTracking();
		void quitTracking();
		void createViewports();

		OIS::Keyboard* mKeyboard;
		OIS::Mouse* mMouse;

		Ogre::Root* mRoot;
        Ogre::SceneManager *mSceneMgr;

		Ogre::RenderWindow* mWindow;
		Ogre::RenderWindow* mSmallWindow;

		bool mShutdown;

		WebcamScene* mScene;

		bool mRiftAvailable;
		bool mTrackingAvailable;
		ARLib::Rift* mRift;
		ARLib::TrackingManager* mTracker;
		ARLib::RenderTarget* mRenderTarget;
		ARLib::RenderTarget* mSmallRenderTarget;

        OgreBulletCollisions::DebugDrawer *mDebugDrawer;
        OgreBulletDynamics::DynamicsWorld *mDynamicsWorld;
        OgreBulletCollisions::CollisionShape *mGroundShape;
};

#endif

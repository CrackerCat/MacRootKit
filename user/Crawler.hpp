#ifndef __CRAWLER_HPP_
#define __CRAWLER_HPP_

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>

#include <UIKit/UIKit.h>
#include <Foundation/Foundation.h>
#include <SpriteKit/SpriteKit.h>

/* iOS App Crawler for macOS */
/*****************************/
/* Crawler will use depth first search */
/* Store previously crawled UI elements */

extern NSString *kNSDarwinAppCrawlerCrawledViews;

namespace NSDarwin
{
	namespace AppCrawler
	{
		class CrawlManager;
	}
}

using namespace NSDarwin::AppCrawler;

@interface NSViewCrawlData : NSObject
	@property (strong, nonatomic) NSString *name;

	@property (strong, nonatomic) NSString *parent;

	@property (assign, nonatomic) CGRect frame;
	@property (assign, nonatomic) CGPoint position; // position in window
	@property (assign, nonatomic) CGPoint center;

	@property (assign, nonatomic) CGPoint anchorPoint;
@end

@interface NSDarwinAppCrawler : NSObject

@property (atomic) CrawlManager *crawlManager;

@property (strong, atomic) NSMutableDictionary *crawlData;

@property (strong, atomic) NSMutableArray *viewControllerStack;

@property (assign, atomic) BOOL spriteKitCrawlCondition;

-(instancetype)initWithCrawlingManager:(CrawlManager*)crawlManager;

-(NSMutableDictionary*)crawlData;

-(NSViewCrawlData*)setupCrawlDataForView:(UIView*)view;

-(BOOL)hasViewBeenCrawled:(UIView*)view inViewController:(UIViewController*)vc;

-(void)crawlingTimerDidFire:(NSTimer*)timer;
-(void)idlingTimerDidFire:(NSTimer*) timer;

-(void)simulateTouchEventAtPoint:(CGPoint)point;
-(void)simulateTouchesOnSpriteKitView:(SKView*)view;

-(void)pushViewControllerToStack:(UIViewController*)vc;

-(BOOL)simulatedTouchesHasHadNoEffect;

@end

namespace NSDarwin
{
	namespace AppCrawler
	{
		class CrawlManager
		{
			public:
				explicit CrawlManager(UIApplication *application, id<UIApplicationDelegate> delegate);

				~CrawlManager();

				NSDarwinAppCrawler* getCrawler() { return crawler; }

				NSTimer* getCrawlingTimer() { return crawlingTimer; }

				NSString* getBundleID() { return bundleIdentifier; }

				UIApplication* getApplication() { return application; }

				id<UIApplicationDelegate> getAppDelegate() { return delegate; }

				UIViewController* getCurrentViewController() { return currentViewController; }

				NSArray* getViews() { return [currentViewController.view subviews]; }

				void setCurrentViewController(UIViewController *viewController) { this->currentViewController = viewController; }

				void setupAppCrawler();

				void setupCrawlingTimer(NSDictionary *userInfo) { this->crawlingTimer = [NSTimer scheduledTimerWithTimeInterval:1.25f
							                                                  target:this->crawler
							                                                selector:@selector(crawlingTimerDidFire:)
							                                                userInfo:userInfo
							                                                 repeats:NO]; }

				void setupIdleTimer() { this->idleTimer = [NSTimer scheduledTimerWithTimeInterval:4.5f
							                                                  target:this->crawler
							                                                selector:@selector(idlingTimerDidFire:)
							                                                userInfo:nil
							                                                 repeats:YES]; }

				void invalidateCrawlingTimer() { if(this->crawlingTimer && [this->crawlingTimer isValid]) { [this->crawlingTimer invalidate]; this->crawlingTimer = NULL; } }

				void invalidateIdleTimer() { if(this->idleTimer && [this->idleTimer isValid]) { [this->idleTimer invalidate]; this->idleTimer = NULL; } }

				NSMutableArray* getViewsForUserInteraction(UIViewController *viewController);
				NSMutableArray* getViewsForUserInteractionFromRootView(UIView *view);
				
				NSMutableArray* getViewsWithKindOfClass(NSMutableArray *views, Class cls);

				void onViewControllerViewDidAppear(UIViewController *viewController);

			private:
				NSDarwinAppCrawler *crawler;

				NSDictionary *crawlData;

				NSString *bundleIdentifier;

				NSTimer *crawlingTimer;
				NSTimer *idleTimer;

				UIApplication *application;

				id<UIApplicationDelegate> delegate;

				UIViewController *currentViewController;

				NSArray *viewControllers;
				NSArray *views;
		};
	}
}

#endif
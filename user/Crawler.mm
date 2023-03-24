#include "Crawler.hpp"

#include "FakeTouch/UIApplication-KIFAdditions.h"
#include "FakeTouch/UIEvent+KIFAdditions.h"
#include "FakeTouch/UITouch-KIFAdditions.h"

#include <assert.h>

using namespace NSDarwin::AppCrawler;

NSString *kNSDarwinAppCrawlerCrawledViews = @"NSDarwinCrawledViews";

static CrawlManager *crawler = NULL;

static bool NSDarwinAppCrawlerClassContainsAdsPrefix(NSString *className)
{
	return [className hasPrefix:@"GAD"] ||
		   [className hasPrefix:@"GAM"];
}

static void NSDarwinAppCrawler_viewDidAppear(id self_, SEL cmd_, BOOL animated)
{
	NSDarwinAppCrawler *darwinCrawler = crawler->getCrawler();

	objc_msgSend(self_, @selector(swizzled_viewDidAppear:), animated);

	crawler->onViewControllerViewDidAppear((UIViewController*) self_);

	[darwinCrawler setSpriteKitCrawlCondition:NO];

	crawler->setCurrentViewController((UIViewController*) self_);
}

static void NSDarwinAppCrawler_viewWillDisappear(id self_, SEL cmd_, BOOL animated)
{
	objc_msgSend(self_, @selector(swizzled_viewWillDisappear:), animated);
}

static void NSDarwinAppCrawler_didMoveToParentViewController(id self_, SEL cmd_, UIViewController *parent)
{
	objc_msgSend(self_, @selector(swizzled_didMoveToParentViewController:), parent);
}

@interface UIView (Depth)

-(NSUInteger)depth;

-(BOOL)hasTapGestureRecognizer;

@end
	
@implementation UIView (Depth)

-(NSUInteger)depth
{
	UIView *v = self;

	NSUInteger depth = 0;

	while(v)
	{
		depth++;

		v = [v superview];
	}

	return depth;
}

-(BOOL)hasTapGestureRecognizer
{
	NSArray *gestureRecognizers = self.gestureRecognizers;

	for(UIGestureRecognizer* recognizer in gestureRecognizers)
	{
		if([recognizer isKindOfClass:[UITapGestureRecognizer class]])
			return YES;
	}

	return NO;
}

@end

@interface NSMutableArray (Shuffling)

-(void)shuffle;

@end

@implementation NSMutableArray (Shuffling)

-(void)shuffle
{
    NSUInteger count = [self count];

    if (count <= 1)
    	return;

    for (NSUInteger i = 0; i < count - 1; ++i)
    {
        NSInteger remainingCount = count - i;
        NSInteger exchangeIndex = i + arc4random_uniform((u_int32_t )remainingCount);
        
        [self exchangeObjectAtIndex:i withObjectAtIndex:exchangeIndex];
    }
}

@end

@interface NSViewCrawlData()
{
}

@end

@implementation NSViewCrawlData

@end

@interface NSDarwinAppCrawler ()
{
    CrawlManager *_crawlManager;
    
    NSMutableDictionary *_crawlData;
}

@end

@implementation NSDarwinAppCrawler

-(instancetype)initWithCrawlingManager:(CrawlManager*)crawlManager
{
	self = [super init];
    
    if(!self)
    	return NULL;

    [self setCrawlManager:crawlManager];
    [self setCrawlingTimer:NULL];
    [self setIdleTimer:NULL];

    [self setTimeSinceLastUserInteraction:CFAbsoluteTimeGetCurrent()];

    [self setCrawlData:[[NSMutableDictionary alloc] init]];
    [self setSpriteKitCrawlCondition:NO];
    [self setViewControllerStack:[[NSMutableArray alloc] init]];
    [self setCrawlerTimerDidFire:NO];

    return self;
}

-(void)setCrawlManager:(CrawlManager*)crawlManager
{
	@synchronized (self)
    {
    	_crawlManager = crawlManager;
    }
}

-(CrawlManager*)crawlManager
{
	@synchronized (self)
    {
        return _crawlManager;
    }

    return _crawlManager;
}

-(void)setCrawlData:(NSMutableDictionary*)crawlData
{
    @synchronized (self)
    {
        _crawlData = crawlData;
    }
}

-(NSMutableDictionary*)crawlData
{
    @synchronized (self)
    {
        return _crawlData;
    }

    return _crawlData;
}

-(NSViewCrawlData*)setupCrawlDataForView:(UIView*)view
{
	NSViewCrawlData *crawlData = [[NSViewCrawlData alloc] init];

	crawlData.name = NSStringFromClass([view class]);
	crawlData.parent = NSStringFromClass([[view superview] class]);

	crawlData.frame = view.frame;
	crawlData.position = [[view superview] convertPoint:view.frame.origin toView:view.window];
	crawlData.center = view.center;
	crawlData.anchorPoint = view.layer.anchorPoint;

	return crawlData;
}


-(BOOL)hasViewBeenCrawled:(UIView*)view inViewController:(UIViewController*)vc
{
	UIViewController *viewController = vc;

	NSDictionary *crawlData = [self.crawlData objectForKey:NSStringFromClass([viewController class])];

	NSMutableDictionary *crawledViews = crawlData ? [crawlData objectForKey:kNSDarwinAppCrawlerCrawledViews] : NULL;

	if(!crawledViews)
		return NO;

	NSMutableArray *viewCrawlData = [crawledViews objectForKey:NSStringFromClass([view class])];

	if(!viewCrawlData)
		return NO;

	CGPoint pointInWindow = [[view superview] convertPoint:view.frame.origin toView:view.window];

	for(NSUInteger i = 0; i < [viewCrawlData count]; i++)
	{
		NSViewCrawlData *crawlData = [viewCrawlData objectAtIndex:i];

		if([crawlData.name isEqual:NSStringFromClass([view class])] &&
			CGPointEqualToPoint(crawlData.position, pointInWindow))
		{
			return YES;
		}
	}

	return NO;
}

-(void)crawlingTimerDidFire:(NSTimer*)timer
{
	NSMutableArray *userInfo = [timer userInfo];

	UIViewController *viewController = [self topViewController];

	if(viewController.navigationController)
	{
		viewController = [viewController.navigationController visibleViewController];
	}

	CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();

	NSString *vcClassName = NSStringFromClass([viewController class]);

	NSMutableDictionary *crawlData = [self.crawlData objectForKey:vcClassName];

	NSMutableDictionary *crawledViews;

	if(crawlData)
		crawledViews = [crawlData objectForKey:kNSDarwinAppCrawlerCrawledViews];
	else
	{
		crawlData = [[NSMutableDictionary alloc] init];
		crawledViews = [[NSMutableDictionary alloc] init];

		[self.crawlData setObject:crawlData forKey:vcClassName];

		[crawlData setObject:crawledViews forKey:kNSDarwinAppCrawlerCrawledViews];
	}

	BOOL didUserInteraction = NO;

	NSMutableArray *eligibleViews = self.crawlManager->getViewsForUserInteraction(viewController); // [eligibleViewsInViewController objectForKey:NSStringFromClass([viewController class])];

	[eligibleViews shuffle];

	if([viewController isKindOfClass:objc_getClass("GADFullScreenAdViewController")])
	{
		if([viewController respondsToSelector:@selector(closeButton)])
		{
			UIButton *closeButton = [viewController performSelector:@selector(closeButton)];

			if(closeButton)
			{
				[closeButton sendActionsForControlEvents:UIControlEventTouchUpInside];

				didUserInteraction = YES;

				goto done;
			}
		}
	}

	if([viewController isKindOfClass:objc_getClass("OGAFullScreenViewController")])
	{
		if([viewController respondsToSelector:@selector(displayer)])
		{
			id displayer = [viewController performSelector:@selector(displayer)];

			if([displayer respondsToSelector:@selector(closeButton)])
			{
				UIButton *closeButton = [viewController performSelector:@selector(closeButton)];

				if(closeButton)
				{
					[closeButton sendActionsForControlEvents:UIControlEventTouchUpInside];

					didUserInteraction = YES;

					goto done;
				}
			}
		}
	}

	if((arc4random() % 10) + 1 <= 7)
	{
		for(UIView *view in eligibleViews)
		{
			BOOL crawled = [self hasViewBeenCrawled:view inViewController:viewController];

			if(crawled)
				continue;

			NSString *viewClassName = NSStringFromClass([view class]);

			if(view.userInteractionEnabled)
			{
				if([view isKindOfClass:[SKView class]])
				{
					self.spriteKitCrawlCondition = YES;

					[self simulateTouchesOnSpriteKitView:(SKView*)view];

					if(!self.spriteKitCrawlCondition)
					{
						didUserInteraction = YES;

						break;
					}

					self.spriteKitCrawlCondition = NO;
				} else
				{
					NSMutableArray *viewCrawlData = [crawledViews objectForKey:viewClassName];

					if(!viewCrawlData)
					{
						viewCrawlData = [[NSMutableArray alloc] init];

						[crawledViews setObject:viewCrawlData forKey:viewClassName];
					}

					CGPoint touchPoint = [[view superview] convertPoint:view.frame.origin toView:view.window];

					[viewCrawlData addObject:[self setupCrawlDataForView:view]];

					[self simulateTouchEventAtPoint:touchPoint];

					didUserInteraction = YES;

					goto done;
				}
			}
		}
	}

	if(!didUserInteraction)
	{
		for(UIView *view in eligibleViews)
		{
			NSString *viewClassName = NSStringFromClass([view class]);

			if(view.userInteractionEnabled)
			{
				if([view isKindOfClass:[SKView class]])
				{
					self.spriteKitCrawlCondition = YES;

					[self simulateTouchesOnSpriteKitView:(SKView*)view];

					if(!self.spriteKitCrawlCondition)
					{
						didUserInteraction = YES;

						break;
					}

					self.spriteKitCrawlCondition = NO;
				} else
				{
					CGPoint touchPoint = [[view superview] convertPoint:view.frame.origin toView:view.window];

					[self simulateTouchEventAtPoint:touchPoint];

					didUserInteraction = YES;

					goto done;
				}
			}
		}
	}

done:
	[timer invalidate];

	if(didUserInteraction)
	{
		self.timeSinceLastUserInteraction = CFAbsoluteTimeGetCurrent();
	}

	self.crawlManager->setupIdleTimer();

	self.crawlerTimerDidFire = YES;
}

-(UIViewController*)topViewController
{
    return [self topViewControllerWithRootViewController:
            [UIApplication sharedApplication].keyWindow.rootViewController];
}

-(UIViewController*)topViewControllerWithRootViewController:(UIViewController*)rootViewController
{
    if([rootViewController isKindOfClass:[UINavigationController class]])
    {
        UINavigationController* navigationController = (UINavigationController*)rootViewController;
        
        return [self topViewControllerWithRootViewController:
                navigationController.visibleViewController];
    } else if (rootViewController.presentedViewController)
    {
        UIViewController* presentedViewController = rootViewController.presentedViewController;
       
        return [self topViewControllerWithRootViewController:
                presentedViewController];
    }else if([rootViewController.childViewControllers count] > 0)
    {
        return [self topViewControllerWithRootViewController:[rootViewController.childViewControllers objectAtIndex:[rootViewController.childViewControllers count] - 1]];
    } else
    {
        return rootViewController;
    }
}

-(void)idlingTimerDidFire:(NSTimer*)timer
{
	NSMutableArray *userInfo = [timer userInfo];

	UIViewController *viewController = [self topViewController];

	if(viewController.navigationController)
	{
		viewController = [viewController.navigationController visibleViewController];
	}

	CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();

	NSString *vcClassName = NSStringFromClass([viewController class]);

	NSMutableDictionary *crawlData = [self.crawlData objectForKey:vcClassName];

	NSMutableDictionary *crawledViews;

	if(crawlData)
		crawledViews = [crawlData objectForKey:kNSDarwinAppCrawlerCrawledViews];
	else
	{
		crawlData = [[NSMutableDictionary alloc] init];
		crawledViews = [[NSMutableDictionary alloc] init];

		[self.crawlData setObject:crawlData forKey:vcClassName];

		[crawlData setObject:crawledViews forKey:kNSDarwinAppCrawlerCrawledViews];
	}

	BOOL didUserInteraction = NO;

	NSMutableArray *eligibleViews = self.crawlManager->getViewsForUserInteraction(viewController); // [eligibleViewsInViewController objectForKey:NSStringFromClass([viewController class])];

	if([viewController isKindOfClass:objc_getClass("GADFullScreenAdViewController")])
	{
		if([viewController respondsToSelector:@selector(closeButton)])
		{
			UIButton *closeButton = [viewController performSelector:@selector(closeButton)];

			if(closeButton)
			{
				[closeButton sendActionsForControlEvents:UIControlEventTouchUpInside];

				goto done;
			}
		}
	}

	if([viewController isKindOfClass:objc_getClass("OGAFullScreenViewController")])
	{
		if([viewController respondsToSelector:@selector(displayer)])
		{
			id displayer = [viewController performSelector:@selector(displayer)];

			if([displayer respondsToSelector:@selector(closeButton)])
			{
				UIButton *closeButton = [viewController performSelector:@selector(closeButton)];

				if(closeButton)
				{
					[closeButton sendActionsForControlEvents:UIControlEventTouchUpInside];

					goto done;
				}
			}
		}
	}

	[self pushViewControllerToStack:viewController];

	if([self simulatedTouchesHasHadNoEffect])
	{
		if(viewController.navigationController)
		{
			viewController = viewController.navigationController;

			eligibleViews = self.crawlManager->getViewsForUserInteraction(viewController);
		}

		[eligibleViews sortUsingComparator:^(id obj1, id obj2)
		{
			UIView *view1 = (UIView*) obj1;
			UIView *view2 = (UIView*) obj2;

			NSUInteger depth1 = [view1 depth];
			NSUInteger depth2 = [view2 depth];

			if(depth1 > depth2)
			{
		        return (NSComparisonResult) NSOrderedDescending;
		    }
		 
		    if(depth1 < depth2)
		    {
		        return (NSComparisonResult) NSOrderedAscending;
		    }

		    return (NSComparisonResult) NSOrderedSame;
		}];

		UIView *eligibleView = [eligibleViews objectAtIndex:0];

		if([eligibleView isKindOfClass:[SKView class]])
		{
			self.spriteKitCrawlCondition = YES;

			[self simulateTouchesOnSpriteKitView:(SKView*)eligibleView];

			if(!self.spriteKitCrawlCondition)
			{
				didUserInteraction = YES;
			}

			self.spriteKitCrawlCondition = NO;

			goto done;
		} else
		{
			CGPoint touchPoint = [[eligibleView superview] convertPoint:eligibleView.frame.origin toView:eligibleView.window];

			[self simulateTouchEventAtPoint:touchPoint];

			didUserInteraction = YES;

			goto done;
		}
	}

	if([eligibleViews count] > 10)
		[eligibleViews shuffle];

	if((arc4random() % 10) + 1 <= 3)
	{
		for(UIView *view in eligibleViews)
		{
			BOOL crawled = [self hasViewBeenCrawled:view inViewController:viewController];

			if(crawled)
				continue;

			NSString *viewClassName = NSStringFromClass([view class]);

			if(view.userInteractionEnabled)
			{
				if([view isKindOfClass:[SKView class]])
				{
					self.spriteKitCrawlCondition = YES;

					[self simulateTouchesOnSpriteKitView:(SKView*)view];

					if(!self.spriteKitCrawlCondition)
					{
						didUserInteraction = YES;

						break;
					}

					self.spriteKitCrawlCondition = NO;
				} else
				{
					NSMutableArray *viewCrawlData = [crawledViews objectForKey:viewClassName];

					if(!viewCrawlData)
					{
						viewCrawlData = [[NSMutableArray alloc] init];

						[crawledViews setObject:viewCrawlData forKey:viewClassName];
					}

					CGPoint touchPoint = [[view superview] convertPoint:view.frame.origin toView:view.window];

					[viewCrawlData addObject:[self setupCrawlDataForView:view]];

					[self simulateTouchEventAtPoint:touchPoint];

					didUserInteraction = YES;

					goto done;
				}
			}
		}
	}

	if(!didUserInteraction)
	{
		for(UIView *view in eligibleViews)
		{
			NSString *viewClassName = NSStringFromClass([view class]);

			if(view.userInteractionEnabled)
			{
				if([view isKindOfClass:[SKView class]])
				{
					self.spriteKitCrawlCondition = YES;

					[self simulateTouchesOnSpriteKitView:(SKView*)view];

					if(!self.spriteKitCrawlCondition)
					{
						didUserInteraction = YES;

						break;
					}

					self.spriteKitCrawlCondition = NO;
				} else
				{
					CGPoint touchPoint = [[view superview] convertPoint:view.frame.origin toView:view.window];

					[self simulateTouchEventAtPoint:touchPoint];

					didUserInteraction = YES;

					goto done;
				}
			}
		}
	}

done:
	if(didUserInteraction)
	{
		self.timeSinceLastUserInteraction = CFAbsoluteTimeGetCurrent();
	}

	self.crawlerTimerDidFire = YES;

	self.crawlManager->setupIdleTimer();
}

-(void)simulateTouchEventAtPoint:(CGPoint)point
{
	NSMutableArray *touches = [[NSMutableArray alloc] init];

	// Initiating the touch

	UITouch *touch = [[UITouch alloc] initAtPoint:point inWindow:[UIApplication sharedApplication].keyWindow];
    
    [touch setLocationInWindow:point];

    [touches addObject:touch];
    
    UIEvent *event = [[UIApplication sharedApplication] _touchesEvent];

    [event _clearTouches];
    [event kif_setEventWithTouches:touches];
    
    for(UITouch *touch in touches)
    	[event _addTouch:touch forDelayedDelivery:NO];

    [[UIApplication sharedApplication] sendEvent:event];

    [touch setPhaseAndUpdateTimestamp:UITouchPhaseStationary];

    // Ending the touch

    [touch setLocationInWindow:point];
    [touch setPhaseAndUpdateTimestamp:UITouchPhaseEnded];

    [touches addObject:touch];

    event = [[UIApplication sharedApplication] _touchesEvent];

    [event _clearTouches];
    [event kif_setEventWithTouches:touches];
    
    for(UITouch *touch in touches)
    	[event _addTouch:touch forDelayedDelivery:NO];

    [[UIApplication sharedApplication] sendEvent:event];
}

-(void)simulateTouchesOnSpriteKitNodes:(SKNode*)node inScene:(SKScene*)scene inView:(SKView*)view
{
	if(!self.spriteKitCrawlCondition)
		return;

	for(SKNode *child in [node children])
	{
		CGPoint sceneCoordinates = [child convertPoint:child.frame.origin toNode:scene];
		CGPoint viewCoordinates = [scene convertPointToView:sceneCoordinates];

		CGPoint touchPoint = [[view superview] convertPoint:viewCoordinates toView:view.window];

		if(!self.spriteKitCrawlCondition)
			return;

		[self simulateTouchEventAtPoint:touchPoint];

		[self simulateTouchesOnSpriteKitNodes:child inScene:scene inView:view];
	}

}

-(void)simulateTouchesOnSpriteKitView:(SKView*)view
{
	SKScene *scene = [view scene];

	[self simulateTouchesOnSpriteKitNodes:scene inScene:scene inView:view];
}

-(void)pushViewControllerToStack:(UIViewController*)vc
{
	if([self.viewControllerStack count] < 7)
	{
		[self.viewControllerStack addObject:NSStringFromClass([vc class])];
	} else
	{
		for(NSUInteger i = 0; i < 6; i++)
		{
			[self.viewControllerStack exchangeObjectAtIndex:i withObjectAtIndex:i + 1];
		}

		[self.viewControllerStack replaceObjectAtIndex:6 withObject:NSStringFromClass([vc class])];
	}
}

-(BOOL)simulatedTouchesHasHadNoEffect
{
	if([self.viewControllerStack count] != 7)
		return NO;

	NSString *className = [self.viewControllerStack objectAtIndex:0];

	for(NSString *vcName in self.viewControllerStack)
	{
		if(![className isEqual:vcName])
			return NO;
	}

	return YES;
}

@end

namespace NSDarwin
{

namespace AppCrawler
{

CrawlManager::CrawlManager(UIApplication *application, id<UIApplicationDelegate> delegate)
{
	this->application = application;
	this->delegate = delegate;

	this->setupAppCrawler();
}

CrawlManager::~CrawlManager()
{
	[this->crawler release];
}

void CrawlManager::setupAppCrawler()
{
	this->crawler = [[NSDarwinAppCrawler alloc] initWithCrawlingManager:this];

	this->crawlData = this->crawler.crawlData;

	this->bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];

	Class UIViewController_class = objc_getClass("UIViewController");

	SEL originalSelector = @selector(viewDidAppear:);
	SEL swizzledSelector = @selector(swizzled_viewDidAppear:);

	BOOL didAddMethod = class_addMethod(UIViewController_class,
										swizzledSelector,
										(IMP) NSDarwinAppCrawler_viewDidAppear,
										"@:");

	if(didAddMethod)
	{
		Method originalMethod = class_getInstanceMethod(UIViewController_class ,originalSelector);
		Method swizzledMethod = class_getInstanceMethod(UIViewController_class, swizzledSelector);

		method_exchangeImplementations(originalMethod, swizzledMethod);
	}

	originalSelector = @selector(viewWillDisappear:);
	swizzledSelector = @selector(swizzled_viewWillDisappear:);

	didAddMethod = class_addMethod(UIViewController_class,
										swizzledSelector,
										(IMP) NSDarwinAppCrawler_viewWillDisappear,
										"@:");

	if(didAddMethod)
	{
		Method originalMethod = class_getInstanceMethod(UIViewController_class ,originalSelector);
		Method swizzledMethod = class_getInstanceMethod(UIViewController_class, swizzledSelector);

		method_exchangeImplementations(originalMethod, swizzledMethod);
	}

	originalSelector = @selector(didMoveToParentViewController:);
	swizzledSelector = @selector(swizzled_didMoveToParentViewController:);

	didAddMethod = class_addMethod(UIViewController_class,
										swizzledSelector,
										(IMP) NSDarwinAppCrawler_didMoveToParentViewController,
										"@:");

	if(didAddMethod)
	{
		Method originalMethod = class_getInstanceMethod(UIViewController_class ,originalSelector);
		Method swizzledMethod = class_getInstanceMethod(UIViewController_class, swizzledSelector);

		method_exchangeImplementations(originalMethod, swizzledMethod);
	}
}

NSMutableArray* CrawlManager::getViewsForUserInteraction(UIViewController *viewController)
{
	return this->getViewsForUserInteractionFromRootView([viewController view]);
}

NSMutableArray* CrawlManager::getViewsForUserInteractionFromRootView(UIView *view)
{
	NSMutableArray *views = [[NSMutableArray alloc] init];

	for(UIView *subview in [view subviews])
	{
		NSString *className = NSStringFromClass([subview class]);

		if(NSDarwinAppCrawlerClassContainsAdsPrefix(className))
			continue;

		UIWindow *window = [subview window];


		if([subview isKindOfClass:[UIScrollView class]])
		{
			// A user interaction is best made inside of a UIScrollView
			[views addObjectsFromArray:this->getViewsForUserInteractionFromRootView(subview)];
		} 
		else if(subview.window &&
			   !subview.hidden &&
				subview.userInteractionEnabled &&
				CGRectContainsPoint(subview.window.frame, [[subview superview] convertPoint:subview.frame.origin toView:subview.window]) &&
			   (([subview isKindOfClass:[UIControl class]] && [className containsString:@"Button"]) ||
			   	[subview isKindOfClass:[UIButton class]] ||
			   	[subview isKindOfClass:[UILabel class]] ||
			   	[subview isKindOfClass:[UIImageView class]] ||
			   	[subview isKindOfClass:[UITableViewCell class]] ||
			   	[subview isKindOfClass:[UICollectionViewCell class]] ||
			   	[subview isKindOfClass:[SKView class]]))
		{
			NSMutableArray *subEligibleViews = this->getViewsForUserInteractionFromRootView(subview);

			if([subEligibleViews count])
			{
				[views addObjectsFromArray:subEligibleViews];
			} 

			if(![views containsObject:subview])
				[views addObject:subview];

		} else
		{
			[views addObjectsFromArray:this->getViewsForUserInteractionFromRootView(subview)];
		}
	}

	return views;
}
 
NSMutableArray* CrawlManager::getViewsWithKindOfClass(NSMutableArray *views, Class cls)
{
	NSMutableArray *views_ = [[NSMutableArray alloc] init];

	for(UIView *view in views)
	{
		if([view isKindOfClass:cls])
		{
			[views_ addObject:view];
		}
	}

	return views_;
}

void CrawlManager::onViewControllerViewDidAppear(UIViewController *viewController)
{
	this->setupCrawlingTimer();
	this->setupIdleTimer();
}

}

}

__attribute__((constructor))
static void initializer()
{
	printf("[%s] initializer()\n", __FILE__);

	crawler = new CrawlManager([UIApplication sharedApplication], [[UIApplication sharedApplication] delegate]);
}

__attribute__ ((destructor))
static void finalizer()
{
	printf("[%s] finalizer()\n", __FILE__);;
}
//
//  ViewController.m
//  IoT
//
//  Created by Admin on 12/4/15.
//  Copyright © 2015 Admin. All rights reserved.
//

#import "ViewController.h"

@interface ViewController ()

@property (weak, nonatomic) IBOutlet UIButton *muscButton;
@property (weak, nonatomic) IBOutlet UIButton *ledButton;
@property (weak, nonatomic) IBOutlet UIButton *startButton;
@property (weak, nonatomic) IBOutlet UIButton *rightButton;
@property (weak, nonatomic) IBOutlet UIButton *leftButton;
@property (weak, nonatomic) IBOutlet UIButton *stopButton;

@end

@implementation ViewController

@synthesize muscButton, ledButton, startButton, rightButton, leftButton, stopButton;

#pragma mark View Controller
- (void)viewDidLoad {
    [super viewDidLoad];
    
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    
}

#pragma mark Button Actions
- (IBAction)didSelectStart:(id)sender {
    [startButton setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    [leftButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [rightButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [stopButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    startButton.userInteractionEnabled = NO;
    leftButton.userInteractionEnabled = YES;
    rightButton.userInteractionEnabled = YES;
    stopButton.userInteractionEnabled = YES;
}

- (IBAction)didSelectLeft:(id)sender {
}

- (IBAction)didSelectRight:(id)sender {
}

- (IBAction)didSelectStop:(id)sender {
    [startButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [leftButton setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    [rightButton setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    [stopButton setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    startButton.userInteractionEnabled = YES;
    leftButton.userInteractionEnabled = NO;
    rightButton.userInteractionEnabled = NO;
    stopButton.userInteractionEnabled = NO;
}

- (IBAction)didSelectMusic:(id)sender {
    if ([muscButton.titleLabel.text  isEqual: @"Music Off"]) {
        [muscButton setTitle:@"Music On" forState: UIControlStateNormal];
    }
    else {
        [muscButton setTitle:@"Music Off" forState: UIControlStateNormal];
    }
}

- (IBAction)didSelectLED:(id)sender {
    if ([ledButton.titleLabel.text  isEqual: @"LED Off"]) {
        [ledButton setTitle:@"LED On" forState: UIControlStateNormal];
    }
    else {
        [ledButton setTitle:@"LED Off" forState: UIControlStateNormal];
    }
}


@end

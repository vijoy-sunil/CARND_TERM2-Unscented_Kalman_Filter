[image1]: ./NIS_calculation/NIS_laser.png
[image2]: ./NIS_calculation/NIS_radar.png
[image3]: ./images/dataset1_zin.PNG
[image4]: ./images/dataset2_zin.PNG
[image5]: ./images/dataset1_zout.PNG
[image6]: ./images/dataset2_zout.PNG

## Project: Unscented Kalman Filter
[![Udacity - Self-Driving Car NanoDegree](https://s3.amazonaws.com/udacity-sdc/github/shield-carnd.svg)](http://www.udacity.com/drive)

This project implements a Unscented kalman filter in C++ to estimate the state of a moving object of interest with noisy lidar and radar measurements. Input data consisting of laser measurements (given directly as x and y positions, with some known uncertainty) and radar measurements (given as radius, angle, and radial velocity relative to some fixed measurement site, with some known uncertainty) are combined with a motion model to track a vehicle with much better accuracy than what can be achieved with individual measurements.

The project uses a "constant turn rate and velocity magnitude" (CTRV) process model to carry out the Kalman filter's predict steps. The CTRV tracks a state vector of 5 quantities: `x position`, `y position`, `velocity magnitude`, `yaw angle`, and `yaw rate`. To predict the position from the time of the old measurement to the time of the current measurement, the velocity magnitude and yaw rate are assumed to be constant; however, a random linear (in the direction of the velocity) acceleration and yaw acceleration are assumed to exist at each time interval. Both accelerations are uncorrelated with a mean of zero and a constant variance.

This project involves the Term 2 Simulator which can be downloaded [here](https://github.com/udacity/self-driving-car-sim/releases)

This repository includes two files that can be used to set up and intall [uWebSocketIO](https://github.com/uWebSockets/uWebSockets) for either Linux or Mac systems. For windows you can use either Docker, VMware, or even [Windows 10 Bash on Ubuntu](https://www.howtogeek.com/249966/how-to-install-and-use-the-linux-bash-shell-on-windows-10/) to install uWebSocketIO. Please see [this concept in the classroom](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/0949fca6-b379-42af-a919-ee50aa304e6a/lessons/f758c44c-5e40-4e01-93b5-1a82aa4e044f/concepts/16cf4a78-4fc7-49e1-8621-3450ca938b77) for the required version and installation scripts.

Once the install for uWebSocketIO is complete, the main program can be built and ran by doing the following from the project top directory.

1. mkdir build
2. cd build
3. cmake ..
4. make
5. ./UnscentedKF

Tips for setting up your environment can be found [here](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/0949fca6-b379-42af-a919-ee50aa304e6a/lessons/f758c44c-5e40-4e01-93b5-1a82aa4e044f/concepts/23d376c7-0195-4276-bdf0-e02f1f3c665d)

Here is the main protcol that main.cpp uses for uWebSocketIO in communicating with the simulator.


INPUT: values provided by the simulator to the c++ program

["sensor_measurement"] => the measurment that the simulator observed (either lidar or radar)


OUTPUT: values provided by the c++ program to the simulator

["estimate_x"] <= kalman filter estimated position x
["estimate_y"] <= kalman filter estimated position y
["rmse_x"]
["rmse_y"]
["rmse_vx"]
["rmse_vy"]

---
## Results
The below outputs show the `RMSE` (root mean square error) when the filter is run on Datasets 1 and 2 in the Simulator. The `RED` circles are lidar measurements and the `BLUE` circles are radar measurements. The `GREEN` markers are the car's position as estimated by the Unscented Kalman filter. 

### 1. Dataset 1

![alt text][image3]

### Path taken by the vehicle

![alt text][image5]


### 2. Dataset 2

![alt text][image4]

### Path taken by the vehicle

![alt text][image6]

---
###  Evaluating the noise parameters using (Normalized Innovation Squared) NIS 

A good way to check if the noise values are physically reasonable is to use the NIS statistic. If our chosen variances used in the prediction step are consistent with physical reality, the NIS values computed from the radar measurements should roughly obey a `chi-square distribution` with degrees of freedom equal to the dimension of the radar measurement space (3). 

Similarly, NIS values computed from the laser measurements should roughly obey a chi-square distribution with 2 degrees of freedom. The below plots show the NIS statistic for the radar or lidar measurements along with the corresponding 95% confidence threshold of the chi-square distribution, which is `7.82 for 3 degrees of freedom` (radar) and `5.99 for 2 degrees of freedom` (lidar). If our noise is consistent, we should see roughly 95% of NIS values computed from measurements fall below that confidence threshold

![alt text][image1]

![alt text][image2]

If much more than 5% of the NIS values computed from measurements exceed the threshold, it means that our measurements are actually being drawn from a distribution with a greater variance than we assumed. In other words, we have underestimated the process noise, and should increase it.

If all of the NIS values computed from measurements fall well below the threshold, it means that our measurements are being drawn from a distribution with smaller variance than we assumed. In other words, we have overestimated our process noise, and should decrease it.

### Comparison to Extended Kalman Filter

The Unscented Kalman Filter is an improvement to the Extended kalman filter implementation. The CRTV (Constant Turn Rate and Velocity Magnitude) model used for this project handles velocity much better than the model used for the extended kalman filter. This model also handles non-linear functions better.

The RMSE values comparison (Dataset 1 only):

|  UKF        |  EKF       |  Min Requirement (UKF)|
|:-----------:|:----------:|:---------------------:|
|X  0.0748    | X  0.0973  |  X  0.09              |
|Y  0.0844    | Y  0.0855  |  Y  0.10              |
|VX  0.3527   | VX  0.4513 |  VX  0.40             |
|VY  0.2404   | VY  0.4399 |  VY  0.30             |

---
## Other Important Dependencies
* cmake >= 3.5
  * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1 (Linux, Mac), 3.81 (Windows)
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools](https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./UnscentedKF` Previous versions use i/o from text files.  The current state uses i/o
from the simulator.

## Generating Additional Data

This is optional!

If you'd like to generate your own radar and lidar data, see the
[utilities repo](https://github.com/udacity/CarND-Mercedes-SF-Utilities) for
Matlab scripts that can generate additional data.

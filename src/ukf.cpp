#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
	is_initialized_ = false;
	
	// if this is false, laser measurements will be ignored (except during init)
	use_laser_ = true;

	// if this is false, radar measurements will be ignored (except during init)
	use_radar_ = true;

	// initial state vector
	x_ = VectorXd(5);

	// initial covariance matrix
	P_ = MatrixXd(5, 5);

	// Process noise standard deviation longitudinal acceleration in m/s^2
	std_a_ = 2;

	// Process noise standard deviation yaw acceleration in rad/s^2
	std_yawdd_ = 0.3;

	//DO NOT MODIFY measurement noise values below these are provided by the sensor manufacturer.
	// Laser measurement noise standard deviation position1 in m
	std_laspx_ = 0.15;

	// Laser measurement noise standard deviation position2 in m
	std_laspy_ = 0.15;

	// Radar measurement noise standard deviation radius in m
	std_radr_ = 0.3;

	// Radar measurement noise standard deviation angle in rad
	std_radphi_ = 0.03;

	// Radar measurement noise standard deviation radius change in m/s
	std_radrd_ = 0.3;
	//DO NOT MODIFY measurement noise values above these are provided by the sensor manufacturer.

	//measurement covariance matrix - laser
	R_laser_ = MatrixXd(2,2);
	R_laser_ << std_laspx_ * std_laspx_, 0,
				0, std_laspy_ * std_laspy_;

	//measurement covariance matrix - radar
	R_radar_ = MatrixXd(3,3);
	R_radar_ << std_radr_ * std_radr_, 0, 0,
				0, std_radphi_ * std_radphi_, 0,
				0, 0, std_radrd_ * std_radrd_;
					 
	n_x_ = 5;
	n_aug_ = 7;
	lambda_ = 3 - n_aug_;
	
	weights_ = VectorXd(2*n_aug_ + 1);
		
	//------------------------------------------------------------------------------------------------------
	// NIS calculation and plot source: mcarilli (https://github.com/mcarilli/CarND-Unscented-Kalman-Filter)
	//------------------------------------------------------------------------------------------------------	
	// Open NIS data files
	NISvals_radar_.open( "../NIS_calculation/NISvals_radar.txt", ios::out );
	NISvals_laser_.open( "../NIS_calculation/NISvals_laser.txt", ios::out );

	// Check for errors opening the files
	if( !NISvals_radar_.is_open() )
	{
	cout << "Error opening NISvals_radar.txt" << endl;
	exit(1);
	}

	if( !NISvals_laser_.is_open() )
	{
	cout << "Error opening NISvals_laser.txt" << endl;
	exit(1);
	}	
	//-------------------------------------------------------------------------------------------------------
}

UKF::~UKF() {
	NISvals_radar_.close();
	NISvals_laser_.close();
}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */ 
	if (!is_initialized_) {
			
		// init covariance matrix
		P_ << 	1, 0, 0, 0, 0,
				0, 1, 0, 0, 0,
				0, 0, 1, 0, 0,
				0, 0, 0, 1, 0,
				0, 0, 0, 0, 1;
				
		if (meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_) {
		  
		  //Convert radar from polar to cartesian coordinates and initialize state.
		  float rho = meas_package.raw_measurements_[0];
		  float phi = meas_package.raw_measurements_[1];
		  
		  float px_r = rho * cos(phi); 
		  float py_r = rho * sin(phi);
	  
		  x_ << px_r, py_r, 0, 0, 0;
		 
		}
		else if (meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_) {
		  
		  //Initialize state.
		  float px_l = meas_package.raw_measurements_[0];
		  float py_l = meas_package.raw_measurements_[1];
		  x_ << px_l, py_l, 0, 0, 0;
		}
			  
		previous_timestamp_ = meas_package.timestamp_;
		
		double weights_0 = lambda_/(lambda_ + n_aug_);

		weights_(0) = weights_0;
		for(int i = 1; i < (2*n_aug_ + 1); i++)
		{
			double weight = 0.5/(n_aug_ + lambda_);
			weights_(i) = weight;
		}
		
		is_initialized_ = true;
		return;
	}
	
	float dt = (meas_package.timestamp_ - previous_timestamp_) / 1000000.0;	//dt - expressed in seconds
	previous_timestamp_ = meas_package.timestamp_;
	
	Prediction(dt);
	
	if (meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_) 
		UpdateRadar( meas_package);
	
	if (meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_) 
		UpdateLidar( meas_package );
	
	// print the output
	cout << "x_ = " << x_ << endl;
	cout << "P_ = " << P_ << endl;  
	
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
	/**
	Complete this function! Estimate the object's location. Modify the state
	vector, x_. Predict sigma points, the state, and the state covariance matrix.
	*/
	//Generate sigma points
	MatrixXd Xsig_aug = MatrixXd(n_aug_, 2*n_aug_ + 1);

	VectorXd x_aug = VectorXd(n_aug_); 
	x_aug.head(5) = x_;
	x_aug(5) = 0;
	x_aug(6) = 0;
	
	MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);
	P_aug.fill(0.0);
	P_aug.topLeftCorner(n_x_, n_x_) = P_;
	P_aug(5, 5) = std_a_ * std_a_;
	P_aug(6, 6) = std_yawdd_ * std_yawdd_;

	MatrixXd L = P_aug.llt().matrixL();
	Xsig_aug.col(0) = x_aug;

	for(int i = 0; i < n_aug_; i++)
	{
		Xsig_aug.col(i + 1) = x_aug + sqrt(lambda_ + n_aug_) * L.col(i);
		Xsig_aug.col(i + 1 + n_aug_) = x_aug - sqrt(lambda_ + n_aug_) * L.col(i);
	}

	//Predict sigma points
	Xsig_pred = MatrixXd(n_x_, 2*n_aug_ + 1);
	for(int i = 0; i< 2*n_aug_+1; i++)
	{
		double p_x 		= Xsig_aug(0, i);
		double p_y 		= Xsig_aug(1, i);
		double v 		= Xsig_aug(2, i);
		double yaw 		= Xsig_aug(3, i);
		double yawd 	= Xsig_aug(4, i);
		double nu_a 	= Xsig_aug(5, i);
		double nu_yawdd = Xsig_aug(6, i);

		double px_p, py_p, v_p, yaw_p, yawd_p;
		if(fabs(yawd) > 0.001)
		{
		  px_p = p_x + (v/yawd) * (sin(yaw + yawd * delta_t) - sin(yaw));
		  py_p = p_y + (v/yawd) * (cos(yaw) - cos(yaw + yawd * delta_t));
		}
		else
		{
		  px_p = p_x + v * delta_t * cos(yaw);
		  py_p = p_y + v * delta_t * sin(yaw);
		}

		v_p = v;
		yaw_p = yaw + yawd * delta_t;
		yawd_p = yawd;
	  
		px_p = px_p + 0.5 * nu_a * delta_t * delta_t * cos(yaw);
		py_p = py_p + 0.5 * nu_a * delta_t * delta_t * sin(yaw);
		v_p = v_p + nu_a * delta_t;
		
		yaw_p = yaw_p + 0.5 * nu_yawdd * delta_t * delta_t;
		yawd_p = yawd_p + nu_yawdd * delta_t;

		Xsig_pred(0, i) = px_p;
		Xsig_pred(1, i) = py_p;
		Xsig_pred(2, i) = v_p;
		Xsig_pred(3, i) = yaw_p;
		Xsig_pred(4, i) = yawd_p;
	}

	//Predict Mean and Covariance
	x_.fill(0.0); 
	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
		x_ = x_ + weights_(i) * Xsig_pred.col(i);
	}

	P_.fill(0.); 
	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
		VectorXd x_diff = Xsig_pred.col(i) - x_;
		while(x_diff(3) > M_PI )
		  x_diff(3) -= 2. * M_PI;
		while(x_diff(3) < -M_PI)
		  x_diff(3) += 2. *M_PI;

		P_ = P_ + weights_(i) * x_diff * x_diff.transpose();
	}
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
	//Predict measurement
	int n_z = 2;
	
	VectorXd z_pred = VectorXd(n_z);
	MatrixXd S = MatrixXd(n_z, n_z);

	MatrixXd Zsig = MatrixXd(n_z, 2*n_aug_ + 1);

	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
	  double p_x = Xsig_pred(0, i);
	  double p_y = Xsig_pred(1, i);
	  
	  Zsig(0, i) = p_x;
	  Zsig(1, i) = p_y;
	}
	
	//Predict Mean and Covariance
	z_pred.fill(0.0);
	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
	  z_pred = z_pred + weights_(i) * Zsig.col(i);
	}

	S.fill(0.0);
	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
	  VectorXd z_diff = Zsig.col(i) - z_pred;  
	  S = S + weights_(i) * z_diff * z_diff.transpose();
	}
	
	S = S + R_laser_; 

	//Calculate Kalman gain
	MatrixXd Tc = MatrixXd(n_x_, n_z);
	Tc.fill(0.0);
	
	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
	  VectorXd z_diff = Zsig.col(i) - z_pred;

	  VectorXd x_diff = Xsig_pred.col(i) - x_;
	  
	  Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
	}
	
	MatrixXd K = Tc * S.inverse();
	
	//Update state
	
	VectorXd z = meas_package.raw_measurements_; 
	VectorXd z_diff = z - z_pred;
  
	x_ = x_ + K * z_diff;
	P_ = P_ - K * S * K.transpose();

	//Calculate NIS and write to file
	NIS_radar_ = z_diff.transpose()*S.inverse()*z_diff;
	NISvals_radar_ << NIS_radar_ << endl;	
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */
	//Predict measurement
	int n_z = 3;
	
	VectorXd z_pred = VectorXd(n_z);
	MatrixXd S = MatrixXd(n_z, n_z);

	MatrixXd Zsig = MatrixXd(n_z, 2*n_aug_ + 1);

	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
	  double p_x 	= Xsig_pred(0, i);
	  double p_y 	= Xsig_pred(1, i);
	  double v 		= Xsig_pred(2, i);
	  double yaw 	= Xsig_pred(3, i);
	  
	  double v1 = cos(yaw) * v;
	  double v2 = sin(yaw) * v;
	  
	  Zsig(0, i) = sqrt(p_x * p_x + p_y * p_y);
	  Zsig(1, i) = atan2(p_y,p_x);
	  Zsig(2, i) = (p_x * v1 + p_y * v2)/sqrt(p_x * p_x + p_y * p_y);
	}
	//Predict Mean and Covariance
	z_pred.fill(0.0);
	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
	  z_pred = z_pred + weights_(i) * Zsig.col(i);
	}

	S.fill(0.0);
	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
	  VectorXd z_diff = Zsig.col(i) - z_pred;
	  while(z_diff(1) > M_PI )
		  z_diff(1) -= 2. * M_PI;
	  while(z_diff(1) < -M_PI)
		  z_diff(1) += 2. *M_PI;
	  
	  S = S + weights_(i) * z_diff * z_diff.transpose();
	}
	
	S = S + R_radar_; 

	//Calculate Kalman gain
	MatrixXd Tc = MatrixXd(n_x_, n_z);
	Tc.fill(0.0);
	
	for(int i = 0; i< (2*n_aug_ + 1); i++)
	{
	  VectorXd z_diff = Zsig.col(i) - z_pred;
	  while(z_diff(1) > M_PI )
		  z_diff(1) -= 2. * M_PI;
	  while(z_diff(1) < -M_PI)
		  z_diff(1) += 2. *M_PI;

	  VectorXd x_diff = Xsig_pred.col(i) - x_;
	  while(x_diff(3) > M_PI )
		  x_diff(3) -= 2. * M_PI;
	  while(x_diff(3) < -M_PI)
		  x_diff(3) += 2. *M_PI;
	  
	  Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
	}
	
	MatrixXd K = Tc * S.inverse();
	
	//Update state
	VectorXd z = meas_package.raw_measurements_;
	VectorXd z_diff = z - z_pred;
	while(z_diff(1) > M_PI )
	  z_diff(1) -= 2. * M_PI;
	while(z_diff(1) < -M_PI)
	  z_diff(1) += 2. *M_PI;
  
	x_ = x_ + K * z_diff;
	P_ = P_ - K * S * K.transpose();
	
	//Calculate NIS and write to file
	NIS_laser_ = z_diff.transpose()*S.inverse()*z_diff;
	NISvals_laser_ << NIS_laser_ << endl;
}

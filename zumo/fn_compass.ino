int COMPASS_calibrate(){
	// The highest possible magnetic value to read in any direction is 2047
	// The lowest possible magnetic value to read in any direction is -2047
	LSM303::vector<int16_t> running_min = {32767, 32767, 32767}, running_max = {-32767, -32767, -32767};

	Wire.begin();// Initiate the Wire library and join the I2C bus as a master
	compass.init();// Initiate LSM303
	compass.enableDefault();// Enables accelerometer and magnetometer
	compass.writeReg(LSM303::CRB_REG_M, CRB_REG_M_2_5GAUSS); // +/- 2.5 gauss sensitivity to hopefully avoid overflow problems
	compass.writeReg(LSM303::CRA_REG_M, CRA_REG_M_220HZ);    // 220 Hz compass update rate

	unsigned char index;

	button.waitForButton();

	// To calibrate the magnetometer, the Zumo spins to find the max/min
	// magnetic vectors. This information is used to correct for offsets
	// in the magnetometer data.
	motors.setLeftSpeed(200);
	motors.setRightSpeed(-200);

	for(index = 0; index < CALIBRATION_SAMPLES; index ++){
		// Take a reading of the magnetic vector and store it in compass.m
		compass.read();

		running_min.x = min(running_min.x, compass.m.x);
		running_min.y = min(running_min.y, compass.m.y);

		running_max.x = max(running_max.x, compass.m.x);
		running_max.y = max(running_max.y, compass.m.y);

		delay(50);
	}

	motors.setLeftSpeed(0);
	motors.setRightSpeed(0);

	// Set calibrated values to compass.m_max and compass.m_min
	compass.m_max.x = running_max.x;
	compass.m_max.y = running_max.y;
	compass.m_min.x = running_min.x;
	compass.m_min.y = running_min.y;
}





// Converts x and y components of a vector to a heading in degrees.
// This function is used instead of LSM303::heading() because we don't
// want the acceleration of the Zumo to factor spuriously into the
// tilt compensation that LSM303::heading() performs. This calculation
// assumes that the Zumo is always level.
template <typename T> float heading(LSM303::vector<T> v){
	float x_scaled =  2.0*(float)(v.x - compass.m_min.x) / ( compass.m_max.x - compass.m_min.x) - 1.0;
	float y_scaled =  2.0*(float)(v.y - compass.m_min.y) / (compass.m_max.y - compass.m_min.y) - 1.0;

	float angle = atan2(y_scaled, x_scaled)*180 / M_PI;
	if (angle < 0) angle += 360;
	return angle;
}

// Yields the angle difference in degrees between two headings
float relativeHeading(float heading_from, float heading_to){
	float relative_heading = heading_to - heading_from;

	// constrain to -180 to 180 degree range
	if (relative_heading > 180) relative_heading -= 360;
	if (relative_heading < -180) relative_heading += 360;

	return relative_heading;
}

// Average 10 vectors to get a better measurement and help smooth out
// the motors' magnetic interference.
float averageHeading(){
	LSM303::vector<int32_t> avg = {0, 0, 0};

	for(int i = 0; i < 10; i ++){
		compass.read();
		avg.x += compass.m.x;
		avg.y += compass.m.y;
	}

	avg.x /= 10.0;
	avg.y /= 10.0;

	// avg is the average measure of the magnetic vector.
	return heading(avg);
}

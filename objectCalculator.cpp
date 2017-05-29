/*
 * author: BN
 * date: 2017
 *
 * TODO:
 *  - pass the image in userdata to mousecallback in ALL phase.
 *  - input validation.
 * 	- improve the selection, don't draw more than one rectangle
 * 	- compile with qt support for addOverlay
 */
#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

/*
 * CLASS VARIABLES
 */

// Workaround for drawing line and rectangle on the image.
Mat imgMeasure;
Mat imgPrepare;

// Stores the selected rectangle coordinates.
int xStart, yStart, xEnd, yEnd = -1;

// Show if we start or end rectangle selection.
bool startSelection = true;

// Store the current phase (important to control the inputs):
// 0 - default: no edit available
// 1 - measure: no edit, just line drawing
// 2 - prepare: draw rectangle and image edit allowed
// 3 - selection: select contour
int programPhase = 0;

// Accepted threshold in the prepare phase.
double thresholdValue;

// Line width attributes of the elements.
int lineWidth = 20;
int rectangleWidth = 5;


// Calculate the pixel per inch ratio. Just a simple division.
double calculatePpi(int realLength, double pixelLength) {
	return (double)realLength / pixelLength;
}

// Calculate the new ppi value based on the resize ratio (from the original image to the resized).
double calculateResizedPpi(double originalPpi, Mat originalImg, Mat resizedImg) {
	double ratio = ((double)originalImg.cols / (double)resizedImg.cols + \
					(double)originalImg.rows / (double)resizedImg.rows) / 2;

	return originalPpi * ratio;
}

// Get the lenght of a line in pixel. Basically it's just a Pythagoras theorem.
double getLineLength() {
	// Doesn't really need to use abs, because we raise it.
	int xDiff = abs(xEnd - xStart);
	int yDiff = abs(yEnd - yStart);

	return pow(pow((double)xDiff, 2) + pow((double)yDiff, 2), 0.5);
}

// Check the selection state.
bool isSelectionDone() {
	// Return true if no selection pending and the coordinates more or equal than zero.
	if (startSelection && xStart >= 0 && yStart >= 0 && xEnd >= 0 && yEnd >= 0) {
		return true;
	} else {
		return false;
	}
}

// Creates the rectangle for the crop.
Rect* createRectangle() {
	// We need this method, because if we start the rectangle from the right to left the widht
	// will be negatie and the liv cannot handle it and throw an exception. Same with the height.

	// Temporary variable for coordinate swapping.
	int tempCoord;

	// If the start is bigger than the end
	if (xStart > xEnd) {
		// swap the coordinates.
		tempCoord = xStart;
		xStart = xEnd;
		xEnd = tempCoord;
	}
	if (yStart > yEnd) {
		// swap the coordinates.
		tempCoord  = yStart;
		yStart = yEnd;
		yEnd = tempCoord;
	}
	// Move the starting point with the size of the rectangle's border.
	xStart += rectangleWidth;
	yStart += rectangleWidth;
	// Subtract the rectangle's border.
	int width = xEnd - xStart -rectangleWidth;
	int height = yEnd - yStart -rectangleWidth;

	// If something go negative return a NULL pointer.
	if (width < 1 || height < 1) {
		cout << "return NULL" << endl;
		return NULL;
	}
	// Everything is seems to be ok.
	return new Rect(xStart, yStart, width, height);
}

// Handle the mouse events in the windows.
void mouseCallback(int event, int x, int y, int flags, void* userdata) {

	// Measure a single line in the picture.
	if (programPhase == 1) {
		if (event == EVENT_LBUTTONDOWN) {
			// Just start creating a line.
			if (startSelection) {

				xStart = x;
				yStart = y;

				cout << " Line first coordinates: \tX - " << x << "\tY - " << y << endl;

				// Change the state.
				startSelection = false;

			} else {

				xEnd = x;
				yEnd = y;

				cout << " Line last coordinates: \t\tX - " << x << "\tY - " << y << endl << endl;

				// Set the state back.
				startSelection = true;

				// Derefer the userdata.
				double* pixelLength = (double*)userdata;
				*pixelLength = getLineLength();

				// Draw the line.
				line(imgMeasure, Point(xStart, yStart), Point(xEnd, yEnd), Scalar(0, 0, 255), lineWidth);
			}
		}
	}
	// If image preparation is on.
	else if (programPhase == 2) {
			if (event == EVENT_LBUTTONDOWN) {
				// Just starting the rectangle selection.
				if (startSelection) {

					xStart = x;
					yStart = y;

					cout << " Selection starting coordinates: \tX - " << x << "\tY - " << y << endl;

					// Change the state.
					startSelection = false;

				} else {

					xEnd = x;
					yEnd = y;

					cout << " Selection ending coordinates: \tX - " << x << "\tY - " << y << endl << endl;

					// Set the state back.
					startSelection = true;

					// Derefer the userdata.
					Mat* imgPointer = (Mat*)userdata;

					// Draw the rectangle.
					rectangle(*imgPointer, Point(xStart, yStart), Point(xEnd, yEnd), Scalar(0, 0, 255), rectangleWidth);
				}
			}
		}
}

// Get the ppi from the image by measure a single line.
double getRatio(Mat image) {
	// Shift the phase to the measure.
	programPhase = 1;

	double pixelLength;
	double* pixelLengthPointer = &pixelLength;

	image.copyTo(imgMeasure);

	namedWindow("Measure", WINDOW_NORMAL);
	resizeWindow("Measure", 1200, 900);

	// Set the callback to the measure window.
	setMouseCallback("Measure", mouseCallback, pixelLengthPointer);

	// Magic is on the way!!
	while (true) {
		imshow("Measure", imgMeasure);

		// Wait for a key press event.
		int key = waitKey(50);

		// If space button hitted calculate the ratio and return it.
		if (key == 32) {
			// Cannot let the user continue with unfinished selection.
			if (!startSelection) {
				cout << " Please finish the selection before continue." << endl;
				continue;
			}

			int realLenght;
			cout << endl << " Add the length of the reference object in millimeter: ";
			cin >> realLenght;
			cout << endl;

			if (realLenght == 0 || pixelLength == 0) {
				cout << " Invalid length. Please try again.";
				continue;
			}

			double ppi = calculatePpi(realLenght, pixelLength);

			// Disable the edit.
			programPhase = 0;

			destroyWindow("Measure");

			return ppi;
		}
		// If 'r' button pressed, restore the original state of the image.
		else if (key == 114) {
			image.copyTo(imgMeasure);
		}
		// If ESC hitted return with the default image
		else if (key == 27) {
			exit(-1);
		}
	}

	programPhase = 0;
	return 0;
}

// Resize the image.
Mat resizeImage(Mat image) {
	// Create a new window for resizing.
	namedWindow("Resize", WINDOW_NORMAL);
	resizeWindow("Resize", 1200, 900);

	// The resized image.
	Mat imgResized;

	// Add a trackbar to the resize window and init the variables for scaling.
	double scaleValue;
	int scaleSliderValue = 100;
	createTrackbar("Scale", "Resize", &scaleSliderValue, 100);

	// Previous slider value for check changes.
	int scaleSliderValueOld = 0;

	// Start the loop for handle the trackbar changes.
	while (true) {

		// Don't let the user set the scaleSliderValue to zero.
		scaleSliderValue = (scaleSliderValue == 0) ? 1 : scaleSliderValue;

		scaleValue = scaleSliderValue / 100.0;

		resize(image, imgResized, Size(0,0), scaleValue, scaleValue);
		imshow("Resize", imgResized);

		if (scaleSliderValue != scaleSliderValueOld) {
			// Print the size of the images.
			cout << " Original size:\t" << image.cols << "*" << image.rows << "\t" << "current size:\t" << imgResized.cols << "*" << imgResized.rows << endl;

			// Set the new value.
			scaleSliderValueOld = scaleSliderValue;
		}

		int key = waitKey(100);

		// If space button hitted destroy the resizing window and return the image.
		if (key == 32) {
			destroyWindow("Resize");
			return imgResized;
		}
		// If ESC hitted exit from the program.
		else if (key == 27) {
			exit(-1);
		}
	}
}

// Prepare the image for the contour detection
Mat prepareImage(Mat image) {
	// Shift the phase to the preparation.
	programPhase = 2;

	// Create windows for the preparation.
	namedWindow("Preparation", WINDOW_NORMAL);
	resizeWindow("Preparation", 1200, 900);

	// Convert the image to grayscale.
	cvtColor(image, image, CV_BGR2GRAY);

	image.copyTo(imgPrepare);

	// Backup image which stores the previous modifications.
	Mat imgBackup;
	image.copyTo(imgBackup);

	// Working with this matrix.
	Mat imgTemp;
	image.copyTo(imgTemp);

	// Pointer to our matrix to pass as reference to the mouse callback.
	Mat* imgPointer = &image;

	// Set the callback to the prepare window.
	setMouseCallback("Preparation", mouseCallback, imgPointer);

	int thresholdSliderValue = 100;
	createTrackbar("Binary threshold", "Preparation", &thresholdSliderValue, 255);

	// Let's do some magic.
	while (true) {

		thresholdValue = thresholdSliderValue;

		threshold(image , imgTemp, thresholdValue, 255.0, THRESH_BINARY);

		// Show the current modified image.
		imshow("Preparation", imgTemp);

		// Wait for a key press event.
		int key = waitKey(50);

		// If space button hitted, blur the image then return.
		if (key == 32) {
			blur(imgTemp, imgTemp, Size(3,3));
			// Disable the edit.
			programPhase = 0;

			destroyWindow("Preparation");

			return imgTemp;
		}
		// If 'r' button pressed, restore the original state of the image.
		else if (key == 114) {
			imgBackup.copyTo(image);
			thresholdSliderValue = 100;
		}
		// If 'c' button pressed and there is a finished selection, crop the image.
		else if (key == 99) {
			if (isSelectionDone()) {
				// Get the rect as a pointer first.
				Rect const* rect = createRectangle();
				// If null it means an error occured.
				if (!rect) {
					// Reset the image.
					cout << " Error: please try again the selection and the crop." << endl;
					imgBackup.copyTo(imgTemp);
					cout << " Image restored." << endl;
				} else {
					// If okay deref the pointer and crop the image
					image = image(*rect);
					// then delete the pointer.
					delete rect;
				}

				// Clear the selection.
				xStart = -1;
				yStart = -1;
				xEnd = -1;
				yEnd = -1;
			}
		}
		// If ESC hitted return with the default image
		else if (key == 27) {
			exit(-1);
		}
	}
}

// Find the contours on the prepared image.
void findContours(Mat image, double ppi) {

	Mat canny_output;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	// Show in a window
	namedWindow("Contours", CV_WINDOW_NORMAL);
	resizeWindow("Contours", 1200, 900);

	// Detect edges using canny.
	Canny(image, canny_output, thresholdValue, thresholdValue*2, 3);
	// Find contours
	findContours(canny_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	// Draw contours.
	Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3);
	cvtColor(image, drawing, CV_GRAY2RGB);

	Mat drawingOriginal;
	drawing.copyTo(drawingOriginal);

	Scalar color = Scalar(1, 1, 255);

	int contourId = 0;
	int contourIdPrev = 0;

	imshow("Contours", drawing);

	cout << " " << contours.size() << " contours found!" << endl << endl;

	cout << " Contour " << contourId + 1 << "\t" << "Area: " << contourArea(contours[contourId]) * ppi * ppi << " mm2"
		 << "\tPerimeter: " << arcLength(contours[contourId], true) * ppi << " mm" << endl;

	while (true) {

		int key = waitKey(50);

		// Left arrow.
		if (key == 81) {
			contourId = contourId == 0 ? contourId : contourId - 1;

			drawingOriginal.copyTo(drawing);

			if (contourId != contourIdPrev) {
				cout << " Contour " << contourId + 1 << "\t" << "Area: " << contourArea(contours[contourId]) * ppi * ppi << " mm2"
					 << "\tPerimeter: " << arcLength(contours[contourId], true) * ppi << " mm" << endl;

				contourIdPrev = contourId;
			}
		}
		// Right arrow.
		else if (key == 83) {
			contourId = contourId == contours.size() - 1 ? contourId : contourId + 1;

			drawingOriginal.copyTo(drawing);

			if (contourId != contourIdPrev) {
				cout << " Contour " << contourId + 1 << "\t" << "Area: " << contourArea(contours[contourId]) * ppi * ppi << " mm2"
					 << "\tPerimeter: " << arcLength(contours[contourId], true) * ppi << " mm" << endl;

				contourIdPrev = contourId;
			}
		}
		// If ESC hitted return with the default image
		else if (key == 27) {
			exit(-1);
		}

		drawContours(drawing, contours, contourId, color, 2, 8, hierarchy, 0, Point());

		imshow("Contours", drawing);
	}
}

int main(int argc, char **argv) {

	// Check the input arguments.
	if (argc < 2) {
		cout << "You have to pass the picture's path as the first argument." << endl;
		cout << "E.g.: ./area-calculator /path/to/image/foobar.jpg" << endl;
		return -1;
	}

	// The image, loaded from the first parameter in grayscale mode.
	Mat imgOriginal = imread(argv[1], CV_LOAD_IMAGE_COLOR);

	// Check the image is loaded or not.
	if (imgOriginal.empty()) {
		cout << "Cannot load the picture. Please try again!" << endl;
		return -1;
	}

	cout << "Image succesfully loaded." << endl;

	// Backup the original image.
	Mat imgOriginalBackup;
	imgOriginal.copyTo(imgOriginalBackup);

	// Start Phase I. Here we let the user to measure a reference line on the screen to calculate the pixel per inch ratio.
	cout << endl << "+------------------+" << endl << "| PHASE I: MEASURE |" << endl << "+------------------+" << endl;
	cout << " Left mouse click - select reference line points" << endl << " r - reset lines" \
		 << " Space - done" << endl << " ESC - exit" << endl << endl;

	double ppi = getRatio(imgOriginal);

	// Start Phase II. Here we let the user to resize the image. Don't need too high resolution, because contour finding may fail.
	cout << endl << "+------------------+" << endl << "| PHASE II: RESIZE |" << endl << "+------------------+" << endl;
	cout << " Space - done" << endl << " ESC - exit" << endl << endl;

	Mat imgResized = resizeImage(imgOriginal);

	// Start Phase III. Here we let the user to prepare the image for the contour finiding. For example: set binary threshold, cut the image.
	cout << endl << "+--------------------+" << endl << "| PHASE III: PREPARE |" << endl << "+--------------------+" << endl;
	cout << " r - restore original image" << endl << " Left mouse click - select points for the cropping rectangle" \
		 << endl << " c - crop the image" << endl << " Space - done" << endl << " ESC - exit" <<  endl << endl;

	Mat imgPrepared = prepareImage(imgResized);

	// We need to calculate the pixel per inch ratio of the new/resized image.
	ppi = calculateResizedPpi(ppi, imgOriginal, imgResized);

	// Start Phase IV. This is the last phase. Here we find the contours and let the user to select one
	// and we write the properties of that contour to the stdout.
	cout << endl << "+------------------+" << endl << "| PHASE IV: SELECT |" << endl << "+------------------+" << endl;
	cout << " Left mouse click - select a contour" << endl << " Space - done" << endl << " ESC - exit" << endl << endl;

	findContours(imgPrepared, ppi);


	// Destroy the opened windows before exit.
	destroyAllWindows();

	// Bye-bye!
	return 0;
}

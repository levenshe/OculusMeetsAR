#include "stdafx.h"
#include <conio.h>
#include "EpiGeoEstimation.h"


EpiGeoEstimation::EpiGeoEstimation(void)
{
	calculatedByZMEK=false;
}


EpiGeoEstimation::~EpiGeoEstimation(void)
{
}

void EpiGeoEstimation::fundamentalHartley(vector<Point2f> leftmatch, vector<Point2f> rightmatch){
	vector<DMatch> finalmatches;
	vector<Point2f> finalpointsL,finalpointsR;
	std::vector<uchar> inliers(leftmatch.size(),0);
	F = findFundamentalMat(leftmatch,rightmatch,inliers,FM_RANSAC,3.0,0.99);

	vector<uchar>::const_iterator itIn= inliers.begin();
	int j=0;
	for ( ;itIn!= inliers.end(); ++itIn,j++) {
		if (*itIn) {
			finalpointsL.push_back(leftmatch[j]);
			finalpointsR.push_back(rightmatch[j]);
		}
	}

	F = findFundamentalMat(finalpointsL,finalpointsR,FM_8POINT);

	Size size(960,1280);
	stereoRectifyUncalibrated(leftmatch, rightmatch, F, size, leftHomography, rightHomography, 3.0);
	calculatedByZMEK=false;
}

void EpiGeoEstimation::fundamentalZMEK(vector<Point2f> leftmatch, vector<Point2f> rightmatch){
	//Ransac
	int ransacCycles = 5000;
	int bestConsensSize = 0;
	double threshold = 4;
	Mat bestF;
	int bestRound=0;

	//RANSAC start
	for(int i=0;i<ransacCycles;i++){
		//generate 7 random numbers in the range 0..leftmatch.size()
		int randomNumbers[7];
		for(int j=0;j<7;j++){
			bool isOld=true;
			while(isOld){
				isOld=false;
				int current = rand()%leftmatch.size();
				for(int k=0;k<j;k++){
					if(current==randomNumbers[k]){
						isOld=true;
					}
				}
				if(!isOld){
					randomNumbers[j]=current;
				}
			}
		}
		vector<Point2f> leftRandom, rightRandom;
		for(int j=0;j<7;j++){
			leftRandom.push_back(leftmatch[randomNumbers[j]]);
			rightRandom.push_back(rightmatch[randomNumbers[j]]);
		}
		//compute fundamental matrix with these random matches
		Mat tempF = linearSolution(leftRandom,rightRandom);

		//compute consensussetsize
		int currentConsSize = 0;
		for(unsigned int j=0;j<leftmatch.size();j++){
			if(transformDistance(tempF,leftmatch[j],rightmatch[j])<threshold){
				currentConsSize++;
			}
		}
		if(currentConsSize>bestConsensSize){
			bestConsensSize=currentConsSize;
			bestF=tempF;
			bestRound=i;
		}

	}

	//calculate consensus set
	vector<Point2f> leftConsensus, rightConsensus;
	for(unsigned int i=0;i<leftmatch.size();i++){
		if(transformDistance(bestF,leftmatch[i],rightmatch[i])<threshold){
			leftConsensus.push_back(leftmatch[i]);
			rightConsensus.push_back(rightmatch[i]);
		}
	}

	F=linearSolution(leftConsensus,rightConsensus);
	calculatedByZMEK=true;
}


void EpiGeoEstimation::homographyEstimation(vector<Point2f> leftmatch,vector<Point2f> rightmatch){
	if(leftHomography.rows==0){
		std::cout<<"\nERROR: Fundamental Matrix has not been estimated yet.";
	}
	else{
		if(calculatedByZMEK){
			std::cout<<"\nChoose a method for homography calculation\n1 - Hartley (recommended)\n2 - ZMEK11\n";
			int input=_getch();
			if(input=='1'){
				Size size(960,1280);
				stereoRectifyUncalibrated(leftmatch, rightmatch, F, size, leftHomography, rightHomography, 3.0);
			}
			else if(input=='2'){
				Mat hom1(3,3,CV_64FC1);
				Mat hom2(3,3,CV_64FC1);
				double ff = F.at<double>(2,2)/F.at<double>(1,1);

				hom1.at<double>(0,0)=1.0;
				hom1.at<double>(0,1)=0.0-F.at<double>(2,0);
				hom1.at<double>(0,2)=ff*F.at<double>(1,0);
				hom1.at<double>(1,0)=F.at<double>(2,0);
				hom1.at<double>(1,1)=1.0;
				hom1.at<double>(1,2)=0.0;
				hom1.at<double>(2,0)=0.0-F.at<double>(1,0);
				hom1.at<double>(2,1)=0.0;
				hom1.at<double>(2,2)=1.0;

				hom2.at<double>(0,0)=0.0-F.at<double>(1,2);
				hom2.at<double>(0,1)=F.at<double>(0,2);
				hom2.at<double>(0,2)=ff*F.at<double>(0,1); //horizontal offset
				hom2.at<double>(1,0)=0.0-F.at<double>(0,2);
				hom2.at<double>(1,1)=0.0-F.at<double>(1,2);
				hom2.at<double>(1,2)=F.at<double>(2,2);
				hom2.at<double>(2,0)=F.at<double>(0,1);
				hom2.at<double>(2,1)=F.at<double>(1,1);
				hom2.at<double>(2,2)=1.0;

				leftHomography=hom1;
				rightHomography=hom2;
			}
		}
		else{
			Size size(960,1280);
			stereoRectifyUncalibrated(leftmatch, rightmatch, F, size, leftHomography, rightHomography, 3.0);
		}
	}
}



double EpiGeoEstimation::transformDistance(Mat curF, Point2f left, Point2f right){
	Mat leftV = Mat(3,1,CV_64FC1);
	leftV.at<double>(0,0)=left.x;
	leftV.at<double>(1,0)=left.y;
	leftV.at<double>(2,0)=1;
	Mat rightV = Mat(1,3,CV_64FC1);
	rightV.at<double>(0,0)=right.x;
	rightV.at<double>(0,1)=right.y;
	rightV.at<double>(0,2)=1;
	Mat Fv = curF*leftV;
	Mat Fw = curF.t()*rightV.t();
	Mat product = rightV*Fv;
	double sampson = product.at<double>(0,0);
	sampson *=sampson;
	double denom = Fv.at<double>(0,0)*Fv.at<double>(0,0)+Fv.at<double>(1,0)*Fv.at<double>(1,0)+Fw.at<double>(0,0)*Fw.at<double>(0,0)+Fw.at<double>(1,0)*Fw.at<double>(1,0);

	return sampson/denom;
}

void EpiGeoEstimation::homographyError(vector<Point2f> leftMatches, vector<Point2f> rightMatches){
	double result;
	if(leftHomography.cols==0){
		std::cout<<"\nERROR: Homographies have not been calculated yet.\n";
	}
	else{
		result=0.0;
		Mat A = leftHomography.inv();
		Mat B = rightHomography.inv();
		for(unsigned int i=0;i<leftMatches.size();i++){
			Mat leftV = Mat(3,1,CV_64FC1);
			leftV.at<double>(0,0)=leftMatches[i].x;
			leftV.at<double>(1,0)=leftMatches[i].y;
			leftV.at<double>(2,0)=1.0f;
			Mat rightV = Mat(3,1,CV_64FC1);
			rightV.at<double>(0,0)=rightMatches[i].x;
			rightV.at<double>(1,0)=rightMatches[i].y;
			rightV.at<double>(2,0)=1.0f;
			Mat Av = A*leftV;
			Mat Bw = B*rightV;
			result+=abs(Av.at<double>(1,0)/Av.at<double>(2,0)-Bw.at<double>(1,0)/Bw.at<double>(2,0));
		}
		result/=(double)leftMatches.size();
		std::cout<<"\nHomography error (rectification precision): "<<result<<"\n";
	}
}

Mat EpiGeoEstimation::linearSolution(vector<Point2f> left, vector<Point2f> right){
	Mat A = Mat(left.size(),7,CV_64FC1);
	for(unsigned int i=0; i<left.size();i++){
		float u=left[i].x;
		float v=left[i].y;
		float u2=right[i].x;
		float v2=right[i].y;
		A.at<double>(i,0)=1.0;
		A.at<double>(i,1)=u;
		A.at<double>(i,2)=v2;
		A.at<double>(i,3)=u2-u;
		A.at<double>(i,4)=u2*v;
		A.at<double>(i,5)=v*v2;
		A.at<double>(i,6)=u*v2-u2*v;
	}

	Mat b = Mat(left.size(),1,CV_64FC1);
	for(unsigned int j=0;j<left.size();j++){
		b.at<double>(j,0)=right[j].y-left[j].y;
	}

	Mat At = A.t();
	Mat B= At*A;
	B=B.inv()*At;
	Mat x = B*b;

	Mat tempF = Mat(3,3,CV_64FC1);
	tempF.at<double>(0,0)=0;
	tempF.at<double>(0,1)=x.at<double>(4,0)-x.at<double>(6,0);
	tempF.at<double>(0,2)=x.at<double>(3,0)+x.at<double>(1,0);
	tempF.at<double>(1,0)=x.at<double>(6,0);
	tempF.at<double>(1,1)=x.at<double>(5,0);
	tempF.at<double>(1,2)=-1+x.at<double>(2,0);
	tempF.at<double>(2,0)=-x.at<double>(3,0);
	tempF.at<double>(2,1)=1;
	tempF.at<double>(2,2)=x.at<double>(0,0);

	return tempF;
}
bool EpiGeoEstimation::loadMatrices(const char *fundFile, const char *leftFile, const char *rightFile){
	F = Mat(3,3,CV_64FC1);
	leftHomography = Mat(3,3,CV_64FC1);
	rightHomography = Mat(3,3,CV_64FC1);
	bool success=loadMatrix(fundFile, &F);
	if(success){
		success= loadMatrix(leftFile, &leftHomography);
		leftHomography=leftHomography.inv();
	}
	if(success){
		success= loadMatrix(rightFile, &rightHomography);
		rightHomography=rightHomography.inv();
	}
	return success;
}

bool EpiGeoEstimation::loadMatrix(const char *fileName, Mat *A){
	bool success=true;
	FILE *file;
	fopen_s(&file,fileName, "r");
	if(file)
	{
		int count1 = fscanf_s(file, "%lf %lf %lf\n", &(A->at<double>(0,0)), &(A->at<double>(0,1)), &(A->at<double>(0,2)));
		int count2 = fscanf_s(file, "%lf %lf %lf\n", &(A->at<double>(1,0)), &(A->at<double>(1,1)), &(A->at<double>(1,2)));
		int count3 = fscanf_s(file, "%lf %lf %lf\n", &(A->at<double>(2,0)), &(A->at<double>(2,1)), &(A->at<double>(2,2)));
		fclose(file);
		if(!(count1==3 && count2==3 && count3==3)){
			success=false;
			std::cout<<"\n"<<"ERROR: "<<fileName<<" contains invalid data.\n";
		}
	}
	else{
		success=false;
		std::cout<<"\nERROR: Could not open"<<fileName;
	}
	return success;
}

bool EpiGeoEstimation::saveMatrices(const char *fundFile, const char *leftFile, const char *rightFile){
	bool success=saveMatrix(fundFile, F);
	if(success){
		success= saveMatrix(leftFile, leftHomography.inv());
	}
	if(success){
		success= saveMatrix(rightFile, rightHomography.inv());
	}
	return success;
}

bool EpiGeoEstimation::saveMatrix(const char *fileName, Mat A){
	bool success = true;
	if(A.rows==3 && A.cols==3){
		FILE *file;
		fopen_s(&file,fileName, "w");
		if(file)
		{
			double temp = A.at<double>(1,1);
			fprintf_s(file, "%.30f %.30f %.30f\n", A.at<double>(0,0), A.at<double>(0,1), A.at<double>(0,2));
			fprintf_s(file, "%.30f %.30f %.30f\n", A.at<double>(1,0), A.at<double>(1,1), A.at<double>(1,2));
			fprintf_s(file, "%.30f %.30f %.30f\n", A.at<double>(2,0), A.at<double>(2,1), A.at<double>(2,2));
			fclose(file);
		}
		else{
			success=false;
		}
	}
	else{
		success=false;
		std::cout<<"\nERROR: Epipolar geometry has not been estimated yet.\n";
	}
	return success;
}
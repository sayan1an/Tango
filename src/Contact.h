#ifndef __Contact_h_
#define __Contact_h_

#include <vec3.hpp>
#include <vec4.hpp>
#include <mat4x4.hpp>
#include <mat3x3.hpp>
#include <gtc/quaternion.hpp>
#include <gtx/quaternion.hpp>

#define isnZero(value, threshold) \
        (value <= -threshold || value >= threshold)

#define isZero(value, threshold) \
        (value >= -threshold && value <= threshold)

struct ContactJacobian {
	/* Normal Constraints*/
	glm::dvec3 linN, angN; // Normal constraints
	glm::dvec3 linN_scaledD, angN_scaledD; // Normal constraints scaled by D_row1_inv
	glm::dvec3 linN_scaledM, angN_scaledM; // Normal constraints scaled by inverse mass and inverse inertia

	glm::dvec3 linT1, angT1;
	glm::dvec3 linT1_scaledD, angT1_scaledD;
	glm::dvec3 linT1_scaledM, angT1_scaledM;

	glm::dvec3 linT2, angT2;
	glm::dvec3 linT2_scaledD, angT2_scaledD;
	glm::dvec3 linT2_scaledM, angT2_scaledM;
};

class Contact {
	RigidBody *A;
	RigidBody *B;
public:
	ContactJacobian jA;
	ContactJacobian jB;
	double b_row1_scaledD;
	double b_row2_scaledD;
	double b_row3_scaledD;

	double lambda1;
	double lambda2;
	double lambda3;

	Contact(RigidBody *A, RigidBody *B, const glm::dvec3 &contactPoint, const glm::dvec3 &contactNormal, double bounce, double dt) {
		A->deltaV = glm::dvec3(0,0,0); A->deltaW = glm::dvec3(0,0,0);
		B->deltaV = glm::dvec3(0,0,0); B->deltaW = glm::dvec3(0,0,0);

		lambda1 = lambda2 = lambda3 = 0;

		this->A = A;
		this->B = B;

		glm::dvec3 linImpA = A->getLinearImpulse(dt); glm::dvec3 angImpA = A->getAngularImpulse(dt);
		glm::dvec3 linImpB = B->getLinearImpulse(dt); glm::dvec3 angImpB = B->getAngularImpulse(dt);

		/* compute constraints for Normal direction*/
		jA.linN = -contactNormal; jA.angN = -(A->getRcrossN(contactPoint, contactNormal));
		jB.linN = contactNormal; jB.angN = B->getRcrossN(contactPoint, contactNormal);

		jA.linN_scaledM = A->getScaledByMinv(jA.linN); jA.angN_scaledM = A->getScaledByIinv(jA.angN);
		jB.linN_scaledM = B->getScaledByMinv(jB.linN); jB.angN_scaledM = B->getScaledByIinv(jB.angN);

		double D_row1_inv = glm::dot(jA.linN, jA.linN_scaledM) + glm::dot(jA.angN, jA.angN_scaledM) +
				glm::dot(jB.linN, jB.linN_scaledM) + glm::dot(jB.angN, jB.angN_scaledM);

		if (isZero(D_row1_inv, 1e-6)) {
			std::cerr<<"1:Two Constrained objects colliding..."<<std::endl;
			exit(0);
		}

		D_row1_inv = 1.0 / D_row1_inv;

		jA.linN_scaledD = jA.linN * D_row1_inv; jA.angN_scaledD = jA.angN * D_row1_inv;
		jB.linN_scaledD = jB.linN * D_row1_inv; jB.angN_scaledD = jB.angN * D_row1_inv;

		b_row1_scaledD = glm::dot(jA.linN, linImpA) + glm::dot(jA.angN, angImpA) +
				glm::dot(jB.linN, linImpB) + glm::dot(jB.angN, angImpB) +
		/*bounce*/ bounce * (A->getDotWithV(jA.linN) + A->getDotWithW(jA.angN) + B->getDotWithV(jB.linN) + B->getDotWithW(jB.angN));

		b_row1_scaledD *= D_row1_inv;




		/* Compute constraints for tangential direction 1*/
		glm::dvec3 tangent1;
		if (isnZero(contactNormal.x, 1e-3))
			tangent1 = glm::dvec3(-contactNormal.y-contactNormal.z, contactNormal.x, contactNormal.x);
		else if (isnZero(contactNormal.y, 1e-6))
			tangent1 = glm::dvec3(contactNormal.y, -contactNormal.z - contactNormal.x, contactNormal.y);
		else if (isnZero(contactNormal.z, 1e-16))
			tangent1 = glm::dvec3(contactNormal.z, contactNormal.z, - contactNormal.x - contactNormal.y);
		else {
			std::cerr<<"Contact Normal is zero."<<std::endl;
			exit(0);
		}
		glm::normalize(tangent1);

		jA.linT1 = -tangent1; jA.angT1 = -(A->getRcrossN(contactPoint, tangent1));
		jB.linT1 = tangent1; jB.angT1 = (B->getRcrossN(contactPoint, tangent1));

		jA.linT1_scaledM = A->getScaledByMinv(jA.linT1); jA.angT1_scaledM = A->getScaledByIinv(jA.angT1);
		jB.linT1_scaledM = B->getScaledByMinv(jB.linT1); jB.angT1_scaledM = B->getScaledByIinv(jB.angT1);

		double D_row2_inv = glm::dot(jA.linT1, jA.linT1_scaledM) + glm::dot(jA.angT1, jA.angT1_scaledM) +
						glm::dot(jB.linT1, jB.linT1_scaledM) + glm::dot(jB.angT1, jB.angT1_scaledM);

		if (isZero(D_row2_inv, 1e-6)) {
			std::cerr<<"2:Two Constrained objects colliding..."<<std::endl;
			exit(0);
		}

		D_row2_inv = 1.0 / D_row2_inv;

		jA.linT1_scaledD = jA.linT1 * D_row2_inv; jA.angT1_scaledD = jA.angT1 * D_row2_inv;
		jB.linT1_scaledD = jB.linT1 * D_row2_inv; jB.angT1_scaledD = jB.angT1 * D_row2_inv;

		b_row2_scaledD = glm::dot(jA.linT1, linImpA) + glm::dot(jA.angT1, angImpA) +
						glm::dot(jB.linT1, linImpB) + glm::dot(jB.angT1, angImpB);

		b_row2_scaledD *= D_row2_inv;




		/* Compute constraints for tangential direction 2*/
		glm::dvec3 tangent2 = glm::normalize(glm::cross(contactNormal, tangent1));

		jA.linT2 = -tangent2; jA.angT2 = -(A->getRcrossN(contactPoint, tangent2));
		jB.linT2 = tangent2;  jB.angT2 = (B->getRcrossN(contactPoint, tangent2));

		jA.linT2_scaledM = A->getScaledByMinv(jA.linT2); jA.angT2_scaledM = A->getScaledByIinv(jA.angT2);
		jB.linT2_scaledM = B->getScaledByMinv(jB.linT2); jB.angT2_scaledM = B->getScaledByIinv(jB.angT2);

		double D_row3_inv = glm::dot(jA.linT2, jA.linT2_scaledM) + glm::dot(jA.angT2, jA.angT2_scaledM) +
								glm::dot(jB.linT2, jB.linT2_scaledM) + glm::dot(jB.angT2, jB.angT2_scaledM);

		if (isZero(D_row3_inv, 1e-6)) {
			std::cerr<<"3:Two Constrained objects colliding..."<<std::endl;
			exit(0);
		}

		D_row3_inv = 1.0 / D_row3_inv;

		jA.linT2_scaledD = jA.linT2 * D_row3_inv; jA.angT2_scaledD = jA.angT2 * D_row3_inv;
		jB.linT2_scaledD = jB.linT2 * D_row3_inv; jB.angT2_scaledD = jB.angT2 * D_row3_inv;

		b_row3_scaledD = glm::dot(jA.linT2, linImpA) + glm::dot(jA.angT2, angImpA) +
						glm::dot(jB.linT2, linImpB) + glm::dot(jB.angT2, angImpB);

		b_row3_scaledD *= D_row3_inv;
	}

	void processContact(double mu) {

	    	double lambda_final1 = lambda1 - b_row1_scaledD - glm::dot(jA.linN_scaledD, A->deltaV)
	    		- glm::dot(jA.angN_scaledD, A->deltaW) - glm::dot(jB.linN_scaledD, B->deltaV)
    			- glm::dot(jB.angN_scaledD, B->deltaW);

	    	if (lambda_final1 < 0) lambda_final1 = 0;

	    	double lambda_final2 = lambda2 - b_row2_scaledD - glm::dot(jA.linT1_scaledD, A->deltaV)
    			- glm::dot(jA.angT1_scaledD, A->deltaW) - glm::dot(jB.linT1_scaledD, B->deltaV)
	    		- glm::dot(jB.angT1_scaledD, B->deltaW);

	    	double max_tangent1 = mu * lambda_final1;
	    	if (lambda_final2 < - max_tangent1) lambda_final2 = - max_tangent1;
	    	else if (lambda_final2 > max_tangent1) lambda_final2 = max_tangent1;

	    	double lambda_final3 = lambda3 - b_row3_scaledD - glm::dot(jA.linT2_scaledD, A->deltaV)
	    	    - glm::dot(jA.angT2_scaledD, A->deltaW) - glm::dot(jB.linT2_scaledD, B->deltaV)
	    		- glm::dot(jB.angT2_scaledD, B->deltaW);

	    	double max_tangent2 = mu * lambda_final1;
	    	if (lambda_final3 < - max_tangent2) lambda_final3 = - max_tangent2;
	    	else if (lambda_final3 > max_tangent2) lambda_final3 = max_tangent2;

	    	double delta_lambda1 = lambda_final1 - lambda1;
	    	double delta_lambda2 = lambda_final2 - lambda2;
	    	double delta_lambda3 = lambda_final3 - lambda3;

	    	A->deltaV += jA.linN_scaledM * delta_lambda1 + jA.linT1_scaledM * delta_lambda2 + jA.linT2_scaledM * delta_lambda3;
	    	A->deltaW += jA.angN_scaledM * delta_lambda1 + jA.angT1_scaledM * delta_lambda2 + jA.angT2_scaledM * delta_lambda3;


			B->deltaV += jB.linN_scaledM * delta_lambda1 + jB.linT1_scaledM * delta_lambda2 + jB.linT2_scaledM * delta_lambda3;
	    	B->deltaW += jB.angN_scaledM * delta_lambda1 + jB.angT1_scaledM * delta_lambda2 + jB.angT2_scaledM * delta_lambda3;

	    	lambda1 = lambda_final1;
	    	lambda2 = lambda_final2;
	    	lambda3 = lambda_final3;
	}

};

#endif

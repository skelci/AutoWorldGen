// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
namespace VaribleMatrix
{
	typedef TArray<TArray<double>> VMatrix;

	VMatrix Create(const uint16 Size, const double Value = 0.0);

	VMatrix Add(const VMatrix& A, const VMatrix& B);
	VMatrix Add(const double A, const VMatrix& B);

	VMatrix Subtract(const VMatrix& A, const VMatrix& B);
	VMatrix Subtract(const double A, const VMatrix& B);

	VMatrix Multiply(const VMatrix& A, const VMatrix& B);
	VMatrix Multiply(const double A, const VMatrix& B);

	VMatrix Divide(const VMatrix& A, const VMatrix& B);
	VMatrix Divide(const double A, const VMatrix& B);

	VMatrix Fade(const VMatrix& x, const double a = 2, const double s = 0, const double k = 1);
}


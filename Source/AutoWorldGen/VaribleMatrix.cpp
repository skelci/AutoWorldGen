// Fill out your copyright notice in the Description page of Project Settings.


#include "VaribleMatrix.h"

namespace VaribleMatrix
{
    VMatrix Create(const uint16 Size, const double Value)
    {
        VMatrix Matrix;
        Matrix.SetNum(Size);

        for (uint16 i = 0; i < Size; ++i)
        {
            Matrix[i].SetNum(Size);
            for (uint16 j = 0; j < Size; ++j)
            {
                Matrix[i][j] = Value;
            }
        }

        return Matrix;
    }

    VMatrix Add(const VMatrix& A, const VMatrix& B)
    {
        VMatrix Result;

        if (A.Num() != B.Num() || A[0].Num() != B[0].Num())
        {
			UE_LOG(LogTemp, Error, TEXT("Matrices must have the same dimensions."));
            return Result;
        }

        uint16 Rows = A.Num();
        uint16 Cols = A[0].Num();

        Result.SetNum(Rows);
        for (uint16 i = 0; i < Rows; ++i)
        {
            Result[i].SetNum(Cols);
            for (uint16 j = 0; j < Cols; ++j)
            {
                Result[i][j] = A[i][j] + B[i][j];
            }
        }

        return Result;
    }
    
    VMatrix Add(const double A, const VMatrix& B)
    {
        VMatrix Result;

        uint16 Rows = B.Num();
        uint16 Cols = B[0].Num();

        Result.SetNum(Rows);
        for (uint16 i = 0; i < Rows; ++i)
        {
            Result[i].SetNum(Cols);
            for (uint16 j = 0; j < Cols; ++j)
            {
                Result[i][j] = A + B[i][j];
            }
        }

        return Result;
    }

    VMatrix Subtract(const VMatrix& A, const VMatrix& B)
    {
        VMatrix Result;

        if (A.Num() != B.Num() || A[0].Num() != B[0].Num())
        {
            UE_LOG(LogTemp, Error, TEXT("Matrices must have the same dimensions."));
            return Result;
        }

        uint16 Rows = A.Num();
        uint16 Cols = A[0].Num();

        Result.SetNum(Rows);
        for (uint16 i = 0; i < Rows; ++i)
        {
            Result[i].SetNum(Cols);
            for (uint16 j = 0; j < Cols; ++j)
            {
                Result[i][j] = A[i][j] - B[i][j];
            }
        }

        return Result;
    }

    VMatrix Subtract(const double A, const VMatrix& B)
    {
        VMatrix Result;

        uint16 Rows = B.Num();
        uint16 Cols = B[0].Num();

        Result.SetNum(Rows);
        for (uint16 i = 0; i < Rows; ++i)
        {
            Result[i].SetNum(Cols);
            for (uint16 j = 0; j < Cols; ++j)
            {
                Result[i][j] = A - B[i][j];
            }
        }

        return Result;
    }

    VMatrix Multiply(const VMatrix& A, const VMatrix& B)
    {
        VMatrix Result;

        if (A.Num() != B.Num() || A[0].Num() != B[0].Num())
        {
            UE_LOG(LogTemp, Error, TEXT("Matrices must have the same dimensions."));
            return Result;
        }

        uint16 Rows = A.Num();
        uint16 Cols = A[0].Num();

        Result.SetNum(Rows);
        for (uint16 i = 0; i < Rows; ++i)
        {
            Result[i].SetNum(Cols);
            for (uint16 j = 0; j < Cols; ++j)
            {
                Result[i][j] = A[i][j] * B[i][j];
            }
        }

        return Result;
    }

    VMatrix Multiply(const double A, const VMatrix& B)
    {
        VMatrix Result;

        uint16 Rows = B.Num();
        uint16 Cols = B[0].Num();

        Result.SetNum(Rows);
        for (uint16 i = 0; i < Rows; ++i)
        {
            Result[i].SetNum(Cols);
            for (uint16 j = 0; j < Cols; ++j)
            {
                Result[i][j] = A * B[i][j];
            }
        }

        return Result;
    }

    VMatrix Divide(const VMatrix& A, const VMatrix& B)
    {
        VMatrix Result;

        if (A.Num() != B.Num() || A[0].Num() != B[0].Num())
        {
            UE_LOG(LogTemp, Error, TEXT("Matrices must have the same dimensions."));
            return Result;
        }

        uint16 Rows = A.Num();
        uint16 Cols = A[0].Num();

        Result.SetNum(Rows);
        for (uint16 i = 0; i < Rows; ++i)
        {
            Result[i].SetNum(Cols);
            for (uint16 j = 0; j < Cols; ++j)
            {
                if (B[i][j] != 0.0)
                {
                    Result[i][j] = A[i][j] / B[i][j];
                }
                else
                {
                    // Handle division by zero
                    Result[i][j] = 0.0; // Assign zero or handle as needed
                }
            }
        }

        return Result;
    }

    VMatrix Divide(const double A, const VMatrix& B)
    {
        VMatrix Result;

        uint16 Rows = B.Num();
        uint16 Cols = B[0].Num();

        Result.SetNum(Rows);
        for (uint16 i = 0; i < Rows; ++i)
        {
            Result[i].SetNum(Cols);
            for (uint16 j = 0; j < Cols; ++j)
            {
                if (B[i][j] != 0.0)
                {
                    Result[i][j] = A / B[i][j];
                }
                else
                {
                    // Handle division by zero
                    Result[i][j] = 0.0; // Assign zero or handle as needed
                }
            }
        }

        return Result;
    }

    VMatrix Fade(const VMatrix& x, const double a, const double s, const double k)
    {
        const uint16 Rows = x.Num();
        const uint16 Cols = x[0].Num();

        // Precompute constants outside the loops
        const double _k = -1 / k;
		const double ks = s; // k * s * 1 / k

        VMatrix Result;
        Result.SetNum(Rows);
		for (uint16 i = 0; i < 50; ++i)
		{	
		    UE_LOG(LogTemp, Warning, TEXT("x_=%f, a=%f, _k=%f, ks=%f"), x[i][i], a, _k, ks);
		}
        for (uint16 i = 0; i < Rows; ++i)
        {
            Result[i].SetNumUninitialized(Cols);
            for (uint16 j = 0; j < Cols; ++j)
            {
				const double x_ = x[i][j];
				const double result = 1 / (1 + FMath::Pow(a, _k * x_ + ks));
				Result[i][j] = result;
            }
        }
        for (uint16 i = 0; i < 50; ++i)
        {
            UE_LOG(LogTemp, Warning, TEXT("result=%f"), Result[i][i]);
        }

        return Result;
    }
}

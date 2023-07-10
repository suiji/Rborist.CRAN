// Copyright (C)  2012-2023  Mark Seligman
//
// This file is part of RboristBase.
//
// RboristBase is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// RboristBase is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RboristBase.  If not, see <http://www.gnu.org/licenses/>.

/**
   @file forestWeightR.cc

   @brief C++ interface to R entry for Meinshausen-style forest weighting.

   @author Mark Seligman
 */

#include "forestWeightR.h"
#include "predictbridge.h"
#include "samplerR.h"
#include "leafR.h"
#include "forestR.h"
#include "forestbridge.h"
#include "samplerbridge.h"
#include "leafbridge.h"

#include <memory>
#include <algorithm>


RcppExport SEXP forestWeightRcpp(const SEXP sTrain,
				 const SEXP sSampler,
				 const SEXP sPredict,
				 const SEXP sArgs) {
  BEGIN_RCPP

  List lArgs(sArgs);
  bool verbose = as<bool>(lArgs["verbose"]);
  if (verbose)
    Rcout << "Entering weighting" << endl;

  List lPredict(sPredict);
  NumericMatrix summary(ForestWeightR::forestWeight(List(sTrain), List(sSampler), as<NumericMatrix>(lPredict["indices"]), List(sArgs)));

  if (verbose)
    Rcout << "Weighting completed" << endl;
  
  return summary;
  END_RCPP
}


NumericMatrix ForestWeightR::forestWeight(const List& lTrain,
					  const List& lSampler,
					  const NumericMatrix& indices,
					  const List& lArgs) {
  BEGIN_RCPP

  SamplerBridge samplerBridge(SamplerR::unwrapGeneric(lSampler));
  LeafBridge leafBridge(LeafR::unwrap(lTrain, samplerBridge));
  return transpose(NumericMatrix(SamplerR::countObservations(lSampler),
				 indices.nrow(),
				 PredictBridge::forestWeight(ForestR::unwrap(lTrain),
							     samplerBridge,
							     leafBridge,
							     indices.begin(),
							     indices.nrow(),
							     as<unsigned int>(lArgs["nThread"])).begin()));
  ForestBridge::deInit();
  
  END_RCPP
}
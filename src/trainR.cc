// Copyright (C)  2012-2023   Mark Seligman
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
   @file trainR.cc

   @brief C++ interface to R entry for training.

   @author Mark Seligman
 */

#include "forestbridge.h"
#include "leafbridge.h"
#include "trainR.h"
#include "trainbridge.h"
#include "rleframeR.h"
#include "rleframe.h"
#include "samplerR.h"

bool TrainR::verbose = false;

const string TrainR::strVersion = "version";
const string TrainR::strSignature = "signature";
const string TrainR::strSamplerHash = "samplerHash";
const string TrainR::strPredInfo = "predInfo";
const string TrainR::strPredMap = "predMap";
const string TrainR::strForest = "forest";
const string TrainR::strLeaf = "leaf";
const string TrainR::strDiagnostic = "diag";
const string TrainR::strClassName = "trainArb";

List TrainR::trainInd(const List& lDeframe, const List& lSampler, const List& argList) {
  BEGIN_RCPP

  if (verbose) {
    Rcout << "Beginning training" << endl;
  }

  vector<string> diag;
  TrainBridge trainBridge(RLEFrameR::unwrap(lDeframe), as<double>(argList["autoCompress"]), as<bool>(argList["enableCoproc"]), diag);
  initFromArgs(argList, trainBridge);

  TrainR trainR(lSampler, argList);
  trainR.trainChunks(trainBridge, as<bool>(argList["thinLeaves"]));
  List outList = trainR.summarize(trainBridge, lDeframe, lSampler, diag);

  if (verbose) {
    Rcout << "Training completed" << endl;
  }

  deInit();
  return outList;

  END_RCPP
}


TrainR::TrainR(const List& lSampler, const List& argList) :
  samplerBridge(SamplerR::unwrapTrain(lSampler, argList)),
  nTree(samplerBridge.getNTree()),
  leaf(LeafR()),
  forest(FBTrain(nTree)) {
}


void TrainR::deInit() {
  verbose = false;
  TrainBridge::deInit();
}


void TrainR::consumeInfo(const TrainedChunk* train) {
  NumericVector infoChunk(train->getPredInfo().begin(), train->getPredInfo().end());
  if (predInfo.length() == 0) {
    predInfo = infoChunk;
  }
  else {
    predInfo = predInfo + infoChunk;
  }
}


List TrainR::summarize(const TrainBridge& trainBridge,
		       const List& lDeframe,
		       const List& lSampler,
		       const vector<string>& diag) {
  BEGIN_RCPP
  List trainArb = List::create(
			       _[strVersion] = "", // Stamped by front end.
			       _[strSignature] = lDeframe["signature"],
			       _[strSamplerHash] = lSampler["hash"],
			       _[strPredInfo] = scaleInfo(trainBridge),
			       _[strPredMap] = std::move(trainBridge.getPredMap()),
			       _[strForest] = std::move(forest.wrap()),
			       _[strLeaf] = std::move(leaf.wrap()),
			       _[strDiagnostic] = diag
                      );
  trainArb.attr("class") = strClassName;

  return trainArb;

  END_RCPP
}


NumericVector TrainR::scaleInfo(const TrainBridge& trainBridge) const {
  BEGIN_RCPP

  vector<unsigned int> pm = trainBridge.getPredMap();
  // Temporary IntegerVector copy for subscripted access.
  IntegerVector predMap(pm.begin(), pm.end());

  // Mapbs back from core order and scales info per-tree.
  return as<NumericVector>(predInfo[predMap]) / nTree;

  END_RCPP
}


void TrainR::trainChunks(const TrainBridge& trainBridge,
			  bool thinLeaves) {
  for (unsigned int treeOff = 0; treeOff < nTree; treeOff += treeChunk) {
    auto chunkThis = treeOff + treeChunk > nTree ? nTree - treeOff : treeChunk;
    ForestBridge fb(chunkThis);
    LeafBridge lb(samplerBridge, thinLeaves);
    auto trainedChunk = trainBridge.train(fb, samplerBridge, treeOff, chunkThis, lb);
    consume(fb, lb, treeOff, chunkThis);
    consumeInfo(trainedChunk.get());
  }
}


void TrainR::consume(const ForestBridge& fb,
		      const LeafBridge& lb,
                      unsigned int treeOff,
                      unsigned int chunkSize) {
  double scale = safeScale(treeOff + chunkSize);
  forest.bridgeConsume(fb, treeOff, scale);
  leaf.bridgeConsume(lb, scale);
  
  if (verbose) {
    Rcout << treeOff + chunkSize << " trees trained" << endl;
  }
}
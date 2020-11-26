#!/usr/bin/env groovy

@Library('etn-ipm2-jenkins') _

//We want only release build and deploy in OBS
//We disabled debug build with tests and coverity analysis
import params.CmakePipelineParams
CmakePipelineParams parameters = new CmakePipelineParams()
parameters.debugBuildRunTests = true
parameters.debugBuildRunMemcheck = false

etn_ipm2_build_and_tests_pipeline_cmake(parameters)

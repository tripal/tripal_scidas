<?php


/**
 * @file
 *
 * The Workflows class.
 *
 * The Workflows class interacts with NextflowGKE to manage contents of a NextflowGKE
 * Workflow. The functions in this class correspond to the NextflowGKE API functions
 * and are named similarly to their Python counterparts.
 */

class NextflowGKEWorkflows extends NextflowGKEAPIService {

  /**
   * Initiates a  workflow run
   *
   * Corresponds to an HTTP GET on the NextflowGKE API path:
   *   /workflows
   *
   * @param $params
   *   An associative array containing the input parameters for this function.
   *   The following parameters are available:
   *
   * - uuid: UUID of the workflow run
   * - image: container image for the workflow
   *
   * @return
   *   An array containing all of the workflows in Galaxy.
   */
  public function create($params = array()) {

  }
  public function delete ($params = array()) {

  }
  public function upload($params = array()) {

  }
  public function launch($params = array()) {

  }
  public function log($params = array()) {

  }
  public function download($params = array()) {

  }


}

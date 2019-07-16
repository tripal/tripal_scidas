<?php

/**
 * @file
 * Provides the form for adding a new workflow to the site.
 */

/**
 * Generates the form that lists the available workflows.
 *
 * Allows the site admin to select which ones should be published on the site
 * for end-users to use.
 */
function tripal_scidas_admin_add_workflow_form($form, &$form_state) {

  $workflow_name = '';
  $workflow_path = '';

  if (array_key_exists('values', $form_state)) {
    $workflow_name = $form_state['values']['workflow_name'];
    $workflow_path = $form_state['values']['workflow_path'];
  }

  // Workflow name.
  $form['workflow_name'] = array(
    '#type' => 'textfield',
    '#title' => 'Workflow Name',
    '#description' => t('The name of the workflow.'),
    '#default_value' => $workflow_name,
    '#required' => TRUE,
  );
  $form['workflow_path'] = array(
    '#type' => 'textfield',
    '#title' => 'Workflow URL',
    '#description' => t('The full URL to the workflow (e.g. https://github.com/SystemsGenetics/AnnoTater)'),
    '#default_value' => $workflow_path,
    '#required' => TRUE,
  );

  // Default submit.
  $form['submit'] = array(
    '#type' => 'submit',
    '#name' => 'submit',
    '#value' => t('Add Workflow'),
  );
  return $form;
}

/**
 * Implements hook_form_validate().
 */
function tripal_scidas_admin_add_workflow_form_validate($form, &$form_state) {
  // Not implemented yet.
}

/**
 * Implements hook_form_submit().
 *
 * Possible conditions:
 *   - Workflow already exists on site and needs to be updated.
 *   - Workflow does not exist on site and needs to be inserted.
 */
function tripal_scidas_admin_add_workflow_form_submit($form, &$form_state) {
  $workflow_name = $form_state['values']['workflow_name'];
  $workflow_path = $form_state['values']['workflow_path'];

  db_insert('tripal_scidas_nfworkflow')
    ->fields([
      'workflow_name' => $workflow_name,
      'workflow_path' => $workflow_path,
    ])
    ->execute();

  // Return to the previous page.
  drupal_goto("/admin/tripal/extension/scidas/workflows");
}

<?php

/**
 * @file
 * This file contains the functions used for administration of the module.
 */

/**
 * Admin home page for Tripal NextflowGKE.
 *
 * @return string
 *   Formatted HTML table containing information on all NextflowGKE Instances.
 */
function tripal_scidas_admin_home() {

  // Initialize the headers and rows of the table.
  $rows = array();
  $headers = array(
    'Server',
    'Description',
    'Status',
    'Options',
  );

  // Retrieve the list of scidas servers that have been added and generate
  // the rows for the table.
  $sql = "SELECT * FROM {tripal_scidas}";
  $results = db_query($sql);

  while ($result = $results->fetchObject()) {
    $rows[] = array(
      $result->servername,
      $result->description,
      $result->serverstatus,
      l(t('edit'), '/admin/tripal/extension/scidas/edit/' . $result->scidas_id) . ' | ' . l(t('test'), '/admin/tripal/extension/scidas/test/' . $result->scidas_id),
    );
  }

  // Theme the table and return.
  $table = array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(
      'id' => 'tripal_scidas-table-properties',
      'class' => 'tripal-data-table',
    ),
    'sticky' => FALSE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => 'No SciDAS instances have been added.',
  );
  return theme_table($table);
}

/**
 * The themable function for the available workflow form.
 */
function theme_tripal_scidas_admin_workflows_form_rows($variables) {
  $form_rows = $variables['form'];

  // The apparopriate headers for this form's table.
  $headers = array(
    t('Enabled'),
    t('Workflow Name'),
    t('NextflowGKE Server'),
    t('Workflow ID'),
    t('Actions'),
  );

  // Theme the rows of this form's table.
  $rows = array();
  foreach (element_children($form_rows) as $i) {
    $row = $form_rows[$i];
    $rows[] = array(
      drupal_render($row["enabled-" . $i]),
      drupal_render($row["workflow_name-user" . $i]),
      drupal_render($row["servername-" . $i]),
      drupal_render($row["workflow_id-user" . $i]),
      drupal_render($row["actions-" . $i]),
    );
  }

  // Subset sequences.
  return theme_table(array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(),
    'sticky' => TRUE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => t('There are no workflows'),
  ));
}

/**
 * Removes a workflow based on the given workflow id.
 *
 * This function is called by Drupal's confirm_form which is setup in the
 * tripal_scidas_admin_confirm_remove_workflow_form() function.
 */
function tripal_scidas_admin_remove_workflow($form, &$form_state) {
  $transaction = db_transaction();

  try {
    $workflow = $form['#workflow'];

    // Delete the table instance given the scidas_workflow_id.
    $success = db_delete('tripal_scidas_nfworkflow')
      ->condition('scidas_nfworkflow_id', $workflow->scidas_nfworkflow_id)
      ->execute();
    $success = db_delete('tripal_scidas_nfworkflow_submission')
      ->condition('scidas_nfworkflow_id', $workflow->scidas_nfworkflow_id)
      ->execute();
  }
  catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot remove the workflow'), 'error');
    drupal_goto("/admin/tripal/extension/scidas/workflows");
    return;
  }

  // Set message and go back to the workflow page.
  drupal_set_message(t('Workflow Deleted Successfully'));
  drupal_goto("/admin/tripal/extension/scidas/workflows");
}

/**
 * Form callback: confirmation form for deleting a tripal_entity.
 */
function tripal_scidas_admin_confirm_remove_workflow_form($form, &$form_state, $scidas_nfworkflow_id) {
  // Get the Node that belongs to this workflow.
  $workflow = db_select('tripal_scidas_nfworkflow', 'tgw')->fields('tgw')
    ->condition('scidas_nfworkflow_id', $scidas_nfworkflow_id)
    ->execute()
    ->fetchObject();

  $form['#workflow'] = $workflow;
  $form['#submit'][] = 'tripal_scidas_admin_remove_workflow';

  $form = confirm_form(
    // Form.
    $form,
    // Question.
    t('Confirm deletion of the workflow titled: %title', array(
      '%title' => $workflow->workflow_name,
    )),
    // The page to go to if the user cancels the action.
    '/admin/tripal/extension/scidas/workflows',
    // Description.
    '<p>' . t('This action cannot be undone and all job submissions for this workflow will be lost.') . '</p>',
    // Yes text.
    t('Delete'),
    // No text.
    t('Cancel'),
    // Internal name for this confirm_form.
    'confirm');

  return $form;
}

/**
 * Responds to the menu item admin/tripal/extension/scidas/test/%.
 *
 * @param int $scidas_id
 *   A scidas server ID.
 */
function tripal_scidas_admin_test_server(int $scidas_id) {
  $status = tripal_scidas_test_connection(array(
    'scidas_id' => $scidas_id,
  ));
  if ($status === FALSE) {
    // Update status in the db.
    db_update('tripal_scidas')->fields(array(
      'serverstatus' => 'unavailable',
    ))
      ->condition('scidas_id', $scidas_id, '=')
      ->execute();
  }
  elseif ($status === TRUE) {
    // Update status in the db.
    db_update('tripal_scidas')->fields(array(
      'serverstatus' => 'available',
    ))
      ->condition('scidas_id', $scidas_id, '=')
      ->execute();
  }
  drupal_goto('admin/tripal/extension/scidas');
}

/**
 * Responds to the menu item admin/tripal/extension/scidas/disable/%.
 *
 * @param int $scidas_id
 *   A scidas server ID.
 */
function tripal_scidas_admin_disable_workflows(int $scidas_id) {
  $transaction = db_transaction();
  try {
    $sql = "SELECT * FROM {tripal_scidas_nfworkflow} WHERE scidas_id = $scidas_id";
    $results = db_query($sql);

    while ($result = $results->fetchObject()) {
      $sucess = db_update('tripal_scidas_nfworkflow')->fields(array(
        'status' => 'disabled',
      ))
        ->condition('scidas_id', $scidas_id, '=')
        ->execute();
    }
    drupal_set_message(t('All workflows disabled'), 'status');
    drupal_goto("/admin/tripal/extension/scidas");
  }
  catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot disable the workflows'), 'error');
    drupal_goto("/admin/tripal/extension/scidas");
    return;
  }
}

/**
 * Responds to the menu item admin/tripal/extension/scidas/disable/%.
 *
 * @param int $scidas_id
 *   A scidas server ID.
 */
function tripal_scidas_admin_disable_workflow(int $scidas_id) {
  $transaction = db_transaction();
  try {
    db_update('tripal_scidas_nfworkflow')
      ->fields(array(
        'status' => 'disabled',
      ))
      ->condition('scidas_workflow_id', $scidas_id, '=')
      ->execute();

    drupal_set_message(t('Workflow disabled'), 'status');
    drupal_goto("/admin/tripal/extension/scidas/workflows");
  }
  catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot disable the workflow'), 'error');
    drupal_goto("/admin/tripal/extension/scidas/workflows");
    return;
  }
}

/**
 * Responds to the menu item admin/tripal/extension/enable/test/%.
 *
 * @param int $scidas_id
 *   A scidas server ID.
 */
function tripal_scidas_admin_enable_workflows(int $scidas_id) {
  $transaction = db_transaction();
  try {
    $sql = "SELECT * FROM {tripal_scidas_nfworkflow} WHERE scidas_id = $scidas_id";
    $results = db_query($sql);

    while ($result = $results->fetchObject()) {
      $sucess = db_update('tripal_scidas_nfworkflow')->fields(array(
        'status' => 'Active',
      ))
        ->condition('scidas_id', $scidas_id, '=')
        ->execute();
    }
    drupal_set_message(t('All workflows enabled'), 'status');
    drupal_goto("/admin/tripal/extension/scidas");
  }
  catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot disable the workflows'), 'error');
    drupal_goto("/admin/tripal/extension/scidas");
    return;
  }
}

/**
 * Responds to the menu item admin/tripal/extension/enable/test/%.
 *
 * @param int $scidas_id
 *   A scidas server ID.
 */
function tripal_scidas_admin_enable_workflow(int $scidas_id) {
  $transaction = db_transaction();
  try {
    $sucess = db_update('tripal_scidas_nfworkflow')
      ->fields(array(
        'status' => 'Active',
      ))
      ->condition('scidas_workflow_id', $scidas_id, '=')
      ->execute();

    drupal_set_message(t('Workflow enabled'), 'status');
    drupal_goto("/admin/tripal/extension/scidas/workflows");
  }
  catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot enable the workflow'), 'error');
    drupal_goto("/admin/tripal/extension/scidas/workflows");
    return;
  }
}

/**
 * Implements hook_form().
 */
function tripal_scidas_admin_workflow_settings_form($form, &$form_state, int $workflow_id) {

  // Set the breadcrumb.
  $breadcrumb = [];
  $breadcrumb[] = l(t('Home'), '<front>');
  $breadcrumb[] = l(t('Administration'), 'admin');
  $breadcrumb[] = l(t('Tripal'), 'admin/tripal');
  $breadcrumb[] = l(t('Extensions'), 'admin/tripal/extension');
  $breadcrumb[] = l(t('SciDAS'), 'admin/tripal/extension/scidas');
  $breadcrumb[] = l(t('Workflows'), 'admin/tripal/extension/scidas/workflows');
  drupal_set_breadcrumb($breadcrumb);

  // Get the workflow
  $workflows = tripal_scidas_get_workflows(['id' => $workflow_id]);
  if (empty($workflows)) {
    drupal_set_message('Invalid workflow ID', 'error');
    return $form;
  }
  $workflow = $workflows[0];

  // Add the workflow to the form
  $form['workflow'] = [
    '#type' => 'value',
    '#value' => $workflow,
  ];

  // Set the page title.
  $name = $workflow->node_title ? $workflow->node_title : $workflow->name;
  drupal_set_title('Workflow Settings: ' . $name);

  // Build the list of tokens
  $header = ['Token', 'Description'];
  $rows = [
    ['[site_name]', 'The name of this site'],
    ['[workflow_title]', 'The title of the workflow.'],
    ['[submission_init]', 'The time when the workflow was submitted to Tripal for execution.'],
    ['[submission_start]', 'The time when the submitted workflow started.'],
    ['[submission_end]', 'The time when the submitted workflow ended.'],
    ['[submission_status]', 'The current status of the sumbitted workflow.'],
    ['[user_name]', 'The name of the user that submitted the workflow.'],
    ['[jobs_link]', "A link to the page where all submitted workflows are listed for the user."],
    ['[results_link]', 'A link to the results page for the workflow. This link is only valid if the workflow completes successfully.'],
  ];
  $tokens = theme('table', array('header' => $header, 'rows' => $rows));

  // Get the defaults
  $defaults = tripal_scidas_get_email_text($workflow);

  // Fieldset for successful workflow completion.
  $form['email_success_fieldset'] = [
    '#type' => 'fieldset',
    '#title' => 'Email Message on Success',
    '#description' => t('Set the subject and body for emails that are sent to users when the workflow successfully completes. You may use the tokens as placeholders for values appropriate for the workflow, site and user. If nothing is provided then the defaults will be used.'),
    '#collapsed' => TRUE,
    '#collapsible' => True,
  ];
  $form['email_success_fieldset']['no_success_email'] = [
    '#type' => 'checkbox',
    '#title' => 'Do not send email on success',
    '#default_value' => $defaults['success']['no_email'],
  ];
  $form['email_success_fieldset']['email_success_subject'] = [
    '#type' => 'textfield',
    '#title' => 'Subject Line',
    '#description' => t('Enter the text that should appear on the subject line.'),
    '#default_value' => $defaults['success']['subject'],
  ];
  $form['email_success_fieldset']['email_success_body'] = [
    '#type' => 'textarea',
    '#title' => 'Email Body',
    '#description' => t('Enter the text that should appear in the body of the email.'),
    '#default_value' => implode("\n", $defaults['success']['body']),
  ];
  $form['email_success_fieldset']['tokens'] = [
    '#type' => 'fieldset',
    '#title' => 'Available Tokens',
    '#description' => t('Use any of these tokens in the subject line or body of your email. Appropriate values will be substituted in their place.'),
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
  ];
  $form['email_success_fieldset']['tokens']['success_tokens'] = [
    '#type' => 'markup',
    '#markup' => $tokens,
  ];

  // Fieldset for failure of workflow.
  $form['email_failure_fieldset'] = [
    '#type' => 'fieldset',
    '#title' => 'Email Message on Failure',
    '#description' => t('Set the subject and body for emails that are sent to users when the workflow fails. You may use the tokens as placeholders for values appropriate for the workflow, site and user. If nothing is provided then the defaults will be used.'),
    '#collapsed' => TRUE,
    '#collapsible' => True,
  ];
  $form['email_failure_fieldset']['no_failure_email'] = [
    '#type' => 'checkbox',
    '#title' => 'Do not send email on failure',
    '#default_value' => $defaults['failure']['no_email'],
  ];
  $form['email_failure_fieldset']['email_failure_subject'] = [
    '#type' => 'textfield',
    '#title' => 'Subject Line',
    '#description' => t('Enter the text that should appear on the subject line.'),
    '#default_value' => $defaults['failure']['subject'],
  ];
  $form['email_failure_fieldset']['email_failure_body'] = [
    '#type' => 'textarea',
    '#title' => 'Email Body',
    '#description' => t('Enter the text that should appear in the body of the email.'),
    '#default_value' => implode("\n", $defaults['failure']['body']),
  ];
  $form['email_failure_fieldset']['tokens'] = [
    '#type' => 'fieldset',
    '#title' => 'Available Tokens',
    '#description' => t('Use any of these tokens in the subject line or body of your email. Appropriate values will be substituted in their place.'),
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
  ];
  $form['email_failure_fieldset']['tokens']['failure_tokens'] = [
    '#type' => 'markup',
    '#markup' => $tokens,
  ];

  // Fieldset for invocation of workflow.
  $form['email_invoke_fieldset'] = [
    '#type' => 'fieldset',
    '#title' => 'Email Message on Success',
    '#description' => t('Set the subject and body for emails that are sent to users when the workflow is invoked. You may use the tokens as placeholders for values appropriate for the workflow, site and user. If nothing is provided then the defaults will be used.'),
    '#collapsed' => TRUE,
    '#collapsible' => True,
  ];
  $form['email_invoke_fieldset']['no_invoke_email'] = [
    '#type' => 'checkbox',
    '#title' => 'Do not send email on workflow invocation.',
    '#default_value' => $defaults['invoke']['no_email'],
  ];
  $form['email_invoke_fieldset']['email_invoke_subject'] = [
    '#type' => 'textfield',
    '#title' => 'Subject Line',
    '#description' => t('Enter the text that should appear on the subject line.'),
    '#default_value' => $defaults['invoke']['subject'],
  ];
  $form['email_invoke_fieldset']['email_invoke_body'] = [
    '#type' => 'textarea',
    '#title' => 'Email Body',
    '#description' => t('Enter the text that should appear in the body of the email.'),
    '#default_value' => implode("\n", $defaults['invoke']['body']),
  ];
  $form['email_invoke_fieldset']['tokens'] = [
    '#type' => 'fieldset',
    '#title' => 'Available Tokens',
    '#description' => t('Use any of these tokens in the subject line or body of your email. Appropriate values will be substituted in their place.'),
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
  ];
  $form['email_invoke_fieldset']['tokens']['invoke_tokens'] = [
    '#type' => 'markup',
    '#markup' => $tokens,
  ];

  $form['submit'] = [
    '#type' => 'submit',
    '#name' => 'Submit',
    '#value' => 'Submit',
  ];

  return $form;

}

/**
 * Implements hook_form_submit().
 */
function tripal_scidas_admin_workflow_settings_form_submit($form, &$form_state) {
  // Get the form values.
  $workflow = $form_state['values']['workflow'];

  $email_failure_subject = trim($form_state['values']['email_failure_subject']);
  $email_failure_body = trim($form_state['values']['email_failure_body']);
  $no_failure_email = trim($form_state['values']['no_failure_email']);

  $email_success_subject = trim($form_state['values']['email_success_subject']);
  $email_success_body = trim($form_state['values']['email_success_body']);
  $no_success_email = trim($form_state['values']['no_success_email']);

  $email_invoke_subject = trim($form_state['values']['email_invoke_subject']);
  $email_invoke_body = trim($form_state['values']['email_invoke_body']);
  $no_invoke_email = trim($form_state['values']['no_invoke_email']);

  // Now store the settings.
  try {
    tripal_scidas_add_workflow_setting($workflow, 'email_failure_subject', $email_failure_subject);
    tripal_scidas_add_workflow_setting($workflow, 'email_failure_body', $email_failure_body);
    tripal_scidas_add_workflow_setting($workflow, 'email_success_subject', $email_success_subject);
    tripal_scidas_add_workflow_setting($workflow, 'email_success_body', $email_success_body);
    tripal_scidas_add_workflow_setting($workflow, 'email_invoke_subject', $email_invoke_subject);
    tripal_scidas_add_workflow_setting($workflow, 'email_invoke_body', $email_invoke_body);
    tripal_scidas_add_workflow_setting($workflow, 'no_invoke_email', $no_invoke_email);
    tripal_scidas_add_workflow_setting($workflow, 'no_success_email', $no_success_email);
    tripal_scidas_add_workflow_setting($workflow, 'no_failure_email', $no_failure_email);
    drupal_set_message('Settings successfully updated');
    drupal_goto('admin/tripal/extension/scidas/workflows');
  }
  catch (Exception $e) {
    drupal_set_message("Could not save the settings. " . $e->getMessage(), 'error');
  }
}

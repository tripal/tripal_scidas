<?php

/**
 * @file
 * The configuration form for adding/deleting a NextflowGKE server.
 */

/**
 * The form for adding a new NextflowGKE server.
 */
function tripal_scidas_admin_configuration_form($form, $form_state, $scidas_id = NULL) {

  // Set the breadcrumb.
  $breadcrumb = array();
  $breadcrumb[] = l(t('Home'), '<front>');
  $breadcrumb[] = l(t('Administration'), 'admin');
  $breadcrumb[] = l(t('Tripal'), 'admin/tripal');
  $breadcrumb[] = l(t('Extensions'), 'admin/tripal/extension');
  $breadcrumb[] = l(t('SciDAS'), 'admin/tripal/extension/scidas');
  drupal_set_breadcrumb($breadcrumb);

  $default_server_name = "";
  $default_user_name = "";
  $default_description = "";
  $default_api_key = "";
  $default_url = "";

  // If a scidas id is set, feel free to declare all the default values with the
  // appropriate
  // NextflowGKE instance.
  if ($scidas_id != NULL) {
    $sql = "SELECT * FROM {tripal_scidas} WHERE scidas_id = :scidas_id";
    $scidas = db_query($sql, array(
      ':scidas_id' => $scidas_id,
    ));
    $scidas = $scidas->fetchObject();
    if ($scidas != NULL) {
      $default_api_key = $scidas->api_key;
      $default_description = $scidas->description;
      $default_server_name = $scidas->servername;
      $default_user_name = $scidas->username;
      $default_url = $scidas->url;
    }
  }

  // The scidas id.
  $form['scidas_id'] = array(
    '#type' => 'value',
    '#value' => $scidas_id,
  );

  // Server Name.
  $form['servername'] = array(
    '#title' => t('NextflowGKE Server Name'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_server_name,
    '#description' => 'Please provide the name of the remote NextflowGKE Server',
    '#required' => TRUE,
  );

  // Description.
  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $default_description,
    '#description' => 'Provide any additional details about this server.',
    '#required' => FALSE,
  );

  // URL.
  $form['url'] = array(
    '#title' => t('URL'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_url,
    '#description' => 'The URL for the remote NextflowGKE server.',
    '#required' => TRUE,
  );

  // User Name.
  $form['username'] = array(
    '#title' => t('User Name'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_user_name,
    '#description' => 'The user name for the NextflowGKE server.
        This username is used to launch all jobs by default.
        If this field is left blank then it is expected that the user
        has an account on the NextflowGKE server and will provide their username
        when executing workflows.',
    '#required' => FALSE,
  );

  // API Key.
  $form['api_key'] = array(
    '#title' => t('API Key'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_api_key,
    '#description' => "The API key for the user name specified above.
        If this field is left blank then it is expected that the user
        will provide their own API key when submitting a job.",
    '#required' => FALSE,
  );

  // Default submit.
  $form['submit'] = array(
    '#type' => 'submit',
    '#name' => 'submit',
    '#value' => t('Submit'),
  );

  // Test Connection.
  $form['test_connect'] = array(
    '#type' => 'submit',
    '#name' => 'test_connect',
    '#value' => t('Test Connection'),
  );

  // Delete.
  if ($scidas_id) {
    $form['delete'] = array(
      '#type' => 'markup',
      '#markup' => l(t('Delete'), 'admin/tripal/extension/scidas/delete/' . $scidas_id),
    );
  }

  return $form;
}

/**
 * The validate hook.
 */
function tripal_scidas_admin_configuration_form_validate($form, &$form_state) {

  // Check to see if we are deleting an instance.
  if ($form_state['clicked_button']['#name'] == 'delete') {
    $server = db_query('tripal_scidas', 'tg')->fields('tg')
      ->condition('servername', $form_state['values']['servername'])
      ->execute()
      ->fetchObject();

    if ($server) {
      // If there is no entry with this servername return False.
      form_set_error('servername', t('A scidas instance with that servername already exists. Please choose another name.'));
      return FALSE;
    }
  }
}

/**
 * The submit hook.
 */
function tripal_scidas_admin_configuration_form_submit($form, &$form_state) {
  global $user;

  // Initialize the data to be sent to the tripal_scidas table.
  $data = array(
    'servername' => trim($form_state['values']['servername']),
    'description' => trim($form_state['values']['description']),
    'url' => trim($form_state['values']['url']),
    'username' => trim($form_state['values']['username']),
    'api_key' => trim($form_state['values']['api_key']),
    'uid' => trim($user->uid),
  );

  if ($form_state['clicked_button']['#name'] == 'submit') {
    // If there is a scidas ID this will be an edit.
    if ($form_state['values']['scidas_id']) {
      $data['scidas_id'] = $form_state['values']['scidas_id'];
      $scidas = drupal_write_record('tripal_scidas', $data, 'scidas_id');
    }
    // Otherwise it's an insert.
    else {
      $scidas = tripal_scidas_add_scidas($data);
    }

    // Insert the new NextflowGKE Instance into Drupal, return where appropriate.
    if (!$scidas) {
      drupal_set_message(t('Problem inserting scidas instance'), 'error');
      return;
    }

    // Default message.
    drupal_set_message(t('NextflowGKE instance has been added'));
    drupal_goto("/admin/tripal/extension/scidas");
  }

  // Else if we are testing the connection.
  else {
    $form_state['rebuild'] = TRUE;

    $connect = tripal_scidas_split_url($data['url']);
    tripal_scidas_test_connection($connect);
  }
}

/**
 * Form to deletes a scidas instance.
 */
function tripal_scidas_admin_delete_scidas_instance_form($form, &$form_state, $scidas_id) {
  $form = array();

  $scidas = db_select('tripal_scidas', 'tg')->fields('tg')
    ->condition('scidas_id', array(
      $scidas_id,
    ))
    ->execute()
    ->fetchObject();

  $form['scidas_id'] = array(
    '#type' => 'value',
    '#value' => $scidas_id,
  );

  return confirm_form($form, t('Confirm removal of the scidas instance: !servername?',
    ['!servername' => $scidas->servername]),
    'admin/tripal/extension/scidas', t('WARNING: removal of a NextflowGKE instance will delete all workflows that have been executed by users of this site. Are you sure you want to do this?'));
}

/**
 * Delete form submit.
 */
function tripal_scidas_admin_delete_scidas_instance_form_submit($form, &$form_state) {
  $scidas_id = $form_state['values']['scidas_id'];

  try {
    // Remove any submissions.
    $sql = "
      DELETE FROM {tripal_scidas_nfworkflow_submission}
      WHERE scidas_workflow_id IN
        (SELECT scidas_workflow_id
         FROM {tripal_scidas_nfworkflow}
         WHERE scidas_id = :scidas_id)";
    $args = array(
      ':scidas_id' => $scidas_id,
    );
    $scidas = db_query($sql, $args);

    // Remove the scidas workflows.
    $sql = "DELETE FROM {tripal_scidas_nfworkflow} WHERE scidas_id = :scidas_id";
    $args = array(
      ':scidas_id' => $scidas_id,
    );
    $scidas = db_query($sql, $args);

    // Remove the NextflowGKE instance itself.
    $sql = "DELETE FROM {tripal_scidas} WHERE scidas_id = :scidas_id";
    $args = array(
      ':scidas_id' => $scidas_id,
    );
    $scidas = db_query($sql, $args);

    drupal_set_message(t('NextflowGKE instance, workflows and submissions have all been removed.'));
  }
  catch (ErrorException $e) {
    drupal_set_message(t('Error deleting record'), 'error');
  }
  drupal_goto("/admin/tripal/extension/scidas");
}

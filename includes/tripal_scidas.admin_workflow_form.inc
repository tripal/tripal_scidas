<?php

/**
 * @file
 * Provides the form for adding workflows.
 */

/**
 * For for adding workflows.
 */
function tripal_scidas_admin_workflows_form($form, &$form_state) {
  $form['#tree'] = TRUE;

  // Get the list of workflows and interate through them.
  $workflows = db_select('tripal_scidas_nfworkflow', 'tgw')->fields('tgw')
    ->orderBy('workflow_name')
    ->execute();
  $rows = [];
  while ($workflow = $workflows->fetchObject()) {
    $remove_link = l(t('Remove'), '/admin/tripal/extension/scidas/workflows/remove/' . $workflow->scidas_nfworkflow_id);
    $rows[] = [
      $workflow->workflow_name,
      $workflow->workflow_path,
      $remove_link
    ];
  }

  // The apparopriate headers for this form's table.
  $headers = array(
    'Workflow Name',
    'Workflow Path',
    'Actions',
  );

  $table = theme_table(array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(),
    'sticky' => TRUE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => 'There are no workflows',
  ));

  $form['workflows'] = [
    '#type' => 'markup',
    '#markup' => $table,
  ];

  return $form;
}



/**
 * Override's Drupal's Hook_form_submit for the available workflow form.
 */
function tripal_scidas_admin_workflows_form_submit($form, &$form_state) {
  $i = 0;
  $form_rows = $form_state['values']['rows'];

  // Go through all of the row entries.
  foreach ($form_rows as $i => $row) {

    // Select the workflow being edited.
    $sql = "SELECT * FROM {tripal_scidas_nfworkflow} WHERE workflow_id = :workflow_id AND scidas_id = :scidas_id";
    $scidas = $form_rows[$i]['scidas-' . $i];
    $scidas_id = $scidas->scidas_id;
    $args = array(
      ':workflow_id' => $form_rows[$i]['workflow_id-' . $i],
      ':scidas_id' => $scidas_id,
    );
    $workflow_instance = db_query($sql, $args);
    $workflow_instance = $workflow_instance->fetchObject();

    // If the workflow has changed values, update it.
    if ($workflow_instance->status != $form_rows[$i]['enabled-' . $i]) {
      $data = array(
        'status' => $form_rows[$i]['enabled-' . $i],
        'scidas_workflow_id' => $workflow_instance->scidas_workflow_id,
        'workflow_name' => $form_rows[$i]['workflow_name-' . $i],
      );
      $workflow = drupal_write_record('tripal_scidas_nfworkflow', $data, 'scidas_workflow_id');
      drupal_set_message(t('Workflow information Updated'));
    }
    $i++;
  }
}

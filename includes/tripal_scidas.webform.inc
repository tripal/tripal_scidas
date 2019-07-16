<?php

/**
 * @file
 * Imports a workflow from a remote Galaxy server and generates a webform.
 */

function tripal_scidas_get_nextflow_params($nextflow_config) {

  // Remove single-line comments
  $nextflow_config = preg_replace('/\/\/.*?\n/','', $nextflow_config);

  // Remove carriage returns
  $nextflow_config = preg_replace('/[\n\r]/', ' ', $nextflow_config);

  // Remove multi-line comments
  $nextflow_config = preg_replace('/\/\*.*?\*\//','', $nextflow_config);

  // Pull out the strings so we don't munge them up and replace them with a token.
  $strings = [];
  preg_match_all('/(?:\"|\')(.*?)(?:\"|\')/', $nextflow_config, $strings);
  $strings = $strings[1];
  foreach ($strings as $i => $string) {
    $string = preg_replace('/([^\w\s])/', "\\\\$1", $string);
    $nextflow_config = preg_replace("/(\"|\')" . $string . "(\"|\')/", 'STR' . $i, $nextflow_config);
  }

  // Tokenize what's left.
  $tokens = preg_split('/\s+/', $nextflow_config);

  // Iterate through the tokens and build a PHP array of the parameters
  $params = [];
  $stack = [];
  $in_params = FALSE;
  for ($i = 0; $i < count($tokens); $i++) {
    if ($tokens[$i] == 'params' && $tokens[$i+1] = '{') {
      $in_params = TRUE;
      $i = $i + 2;
    }
    if ($in_params) {
      if ($tokens[$i] == "}") {
        $in_params = FALSE;
      }
      _tripal_scidas_add_nextflow_param($tokens, $i, $params, $strings);
    }
  }
  return $params;
}

/**
 * A helper recursive function for parsing nextflow.config file.
 * @param unknown $tokens
 * @param unknown $i
 * @param unknown $params
 * @param unknown $strings
 * @param number $depth
 */
function _tripal_scidas_add_nextflow_param($tokens, &$i, &$params, $strings, $depth=0) {

  $curr_token = $tokens[$i];
  $next_token = $tokens[$i+1];

  // If this is an array of values then recurse.
  if ($next_token == '{') {
    $i = $i + 2;
    $subparams = [];
    // Iterate through the elements of this array and get each key/value
    for ($i = $i; $i < count($tokens); $i++) {
      // We've reache the end of the stanza.
      if ($tokens[$i] == "}") {
        break;
      }
      _tripal_scidas_add_nextflow_param($tokens, $i, $subparams, $strings, $depth+1);
    }
    $params[$curr_token] = $subparams;
  }
  // If this is a single key/value pair then get the value and return.
  else if ($next_token == '=') {
    if ($tokens[$i+2] == '{') {
      $value = $tokens[$i+3];
      $params[$curr_token] = _tripal_scidas_add_nextflow_param_details($value, $strings);
      $i = $i + 4;
    }
    else {
      $value = $tokens[$i+2];
      $params[$curr_token] = _tripal_scidas_add_nextflow_param_details($value, $strings);
      $i = $i + 2;
    }
  }
}
/**
 *
 * @param unknown $value
 * @param unknown $strings
 * @return string[]|unknown[]
 */
function _tripal_scidas_add_nextflow_param_details($value, $strings) {

  $type = 'string';
  $matches = [];
  if (preg_match('/STR(\d+)/', $value, $matches)){
    $value = $strings[$matches[1]];
    $type = 'string';
  }
  elseif (preg_match('/^(true|false)$/', $value)){
    $type = 'boolean';
  }
  elseif (preg_match('/^-*(\d+|\d+\.\d+)$/', $value)) {
    $type = 'numeric';
  }

  return [
    'value' => $value,
    'type' => $type,
  ];
}
/**
 * The base function to submitted_list()
 */
function tripal_scidas_build_webform($workflow_id) {

  // We need to keep track of data_input (and data_collection_inputs) so that
  // we can map allowed extensions from tools to these input steps.
  $data_inputs = [];

  // An object that will contain the componentns and conditions that
  // will be used to build the webform.
  $webform = new stdClass();
  $webform->components = [];
  $webform->conditionals = [];

  // Get the configfile for the workflow
  $nfworkflow = db_select('tripal_scidas_nfworkflow', 'tsn')
    ->fields('tsn')
    ->condition('scidas_nfworkflow_id', $workflow_id)
    ->execute()
    ->fetchObject();
  if (!$nfworkflow) {
    drupal_set_message("Could not find the workflow with the given id: " . $workflow_id, 'error');
    return FALSE;
  }

  // Get the Nextflow config file.
  //$config_url = $nfworkflow->workflow_path . '/nextflow.config';
  $config_url = 'https://raw.githubusercontent.com/SystemsGenetics/GEMmaker/master/nextflow.config';
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, $config_url);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
  $response = curl_exec($ch);
  curl_close($ch);

  if (!$response) {
    drupal_set_message("Could not retreive the \"nextflow.config\" file from the remote workflow repository: " . $config_url, 'error');
    return FALSE;
  }

  $params = tripal_scidas_get_nextflow_params($response);

  // How do I extend the node type so it tells me in the form id that this
  // is a 'galaxy' webform?
  $node = new stdClass();
  $node->type = 'webform';
  node_object_prepare($node);
  $node->title = $nfworkflow['workflow_name'];
  $node->language = 'en';
  $node->body[LANGUAGE_NONE][0]['value'] = '';
  $node->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  $node->uid = 1;
  $node->promote = 0;
  $node->comment = 0;

  // Attach the webform to the node.
  $node->webform = [
    'confirmation' => '',
    'confirmation_format' => NULL,
    'redirect_url' => '',
    'status' => '1',
    'block' => '0',
    'teaser' => '1',
    'allow_draft' => '0',
    'auto_save' => '0',
    'submit_notice' => '1',
    'confidential' => '0',
    'submit_text' => '',
    'submit_limit' => '-1',
    'submit_interval' => '-1',
    'total_submit_limit' => '-1',
    'total_submit_interval' => '-1',
    'progressbar_bar' => '1',
    'progressbar_page_number' => '1',
    'progressbar_percent' => '1',
    'progressbar_pagebreak_labels' => '1',
    'progressbar_include_confirmation' => '1',
    'progressbar_label_first' => 'Step 1',
    'progressbar_label_confirmation' => 'Submit',
    'preview' => '1',
    'preview_next_button_label' => 'Next Step',
    'preview_prev_button_label' => 'Previous Step',
    'preview_title' => '',
    'preview_message' => '',
    'preview_message_format' => NULL,
    'record_exists' => TRUE,
    'roles' => [
      '1',
      '2',
    ],
    'emails' => [],
    'components' => $webform->components,
    'conditionals' => $webform->conditionals,
  ];

  dpm($node);

  // Save the node.
  node_save($node);
  // A new node will have been created be sure to record the nid in the
  // workflow table on the db.
  $nid = (int) $node->nid;
  return $nid;
}

/**
 * Adds a single input component or recurses.
 *
 * A function to construct a web form component based on a given tool input Some
 * of these workflows will have its 'vanilla' inputs and others will have its
 * inputs already chosen (workflow might have been constructed on the galaxy
 * server or imported)
 */
function tripal_scidas_build_webform_add_param($webform, $params) {

  foreach ($params as $param_name => $param_value) {
    if (is_array($param_value)) {

      // Each param array is contained in a fieldset. We'll name the field set after
      // it's step and if a tool is present we'll include the tool info.
      $cid = count($webform->components) + 1;
      $fieldset_key = $param_name . "_fieldset";
      $fieldset_name = $param_name;

      // Add the fieldset to the components list.
      $webform->components[] = [
          'cid' => $cid,
          'pid' => '0',
          'form_key' => $fieldset_key,
          'name' => $fieldset_name,
          'type' => 'fieldset',
          'value' => '',
          'extra' => [
              'description' => '',
              'description_above' => 1,
              'private' => 0,
              'css_classes' => 'tripal-scidas-fieldset',
              'title_display' => 0,
              'collapsible' => 1,
              'collapsed' => 0,
          ],
          'required' => '0',
          'weight' => $cid,
      ];

      $fieldset_cid = count($webform->components);

      foreach ($param_value as $param) {
        tripal_scidas_build_webform_add_param($webform, $param);
      }
    }
    else {
      tripal_scidas_build_webform_add_component($webform, $param_name, $param_value);
    }
  }
}

/**
 * Adds a single component to the webform.
 */
function tripal_scidas_build_webform_add_component($webform, $param_name, $param_value) {

  // Initiate variables that will be used for building the webform component.
  $is_list = 1;
  $webform_value = "";
  $webform_type = 'select';
  $is_required = 1;
  $extra_description = '';
  $extra = [
    'title_display' => 'before',
    'description' => $extra_description,
    'description_above' => 0,
    'items' => '',
    'aslist' => $is_list,
  ];

  // Now, set the webform component type and the value depending on the
  // tool input type.
  switch ($param_value['type']) {
    case 'string':
      // Text should be the webform equivalent of textfield.
      $webform_type = 'textfield';
      break;

    case 'data_input':
      // The 'data_input' type is really not a Galaxy tool type. It's the type
      // we created for the 'data_input' step that doesn't have a tool.
      // we want to use this same function to create components so we fake
      // a tool and input and use this type.
      $webform_type = 'galaxy_sfile';

      // Intialize the allowed extension variables. These get set in the
      // appropraite 'data' tool type.
      $extra['filtering']['types'] = [];
      // The webform 'file' component wants a size. Let this default to the
      // server settings.
      $extra['filtering']['size'] = '0';
      // Set the allowed extension as per the rules on Galaxy.
      $extra['filtering']['addextensions'] = '';
      break;

    case 'numeric':
      $webform_type = 'number';
      $webform_value = $param_value['value'];
      break;

    case 'boolean':
      $webform_type = 'select';
      if ($webform_value == 'true') {
        $webform_value = 'true';
      }
      else {
        $webform_value = 'false';
      }
      $extra['items'] = "true|Yes\nfalse|No";
      $extra['aslist'] = 0;

      // Check defaults.
      break;

    default:
      drupal_set_message('Unhandled tool input type: ' . $param_value['type'] . ".", 'error');
      break;
  }

  $cid = count($webform->components) + 1;
  $component = [
    'cid' => $cid,
    'pid' => $pid,
    'name' => $input_name,
    // This form_key has to be unique from ANYTHING else, otherwise the
    // component will not be presented.
    'form_key' => "step_" . ($step['id'] + 2) . '_' . $cid,
    'type' => $webform_type,
    'value' => $webform_value,
    'extra' => $extra,
    'weight' => $cid,
    'required' => $is_required,
  ];
  $webform->components[] = $component;

  return $cid;
}


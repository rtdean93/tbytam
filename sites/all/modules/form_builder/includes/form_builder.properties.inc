<?php
// $Id: form_builder.properties.inc,v 1.16 2011/01/12 04:00:19 quicksketch Exp $

/**
 * @file form_builder.properties.inc
 * Implementations of hook_form_builder_properties in separate functions.
 */

/**
 * Configuration form for the "key" property.
 *
 * The key property is special in that it's not actually part of the element,
 * but instead the array key that is used to reference the element in the
 * Form API structure.
 */
function form_builder_property_key_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['key'] = array(
    '#title' => t('Form key'),
    '#type' => 'textfield',
    '#default_value' => $element['#key'],
    '#required' => TRUE,
    '#weight' => -9,
    '#element_validate' => array('form_builder_property_key_form_validate'),
  );

  return $form;
}

/**
 * Element validate function for the "key" property. Ensure safe characters.
 */
function form_builder_property_key_form_validate($element, $form_state) {
  if (!preg_match('/^[a-z0-9_]+$/', $element['#value'])) {
    form_error($element, t('The form key may only contain lowercase alphanumeric characters and underscores.'));
  }
}

/**
 * Configuration form for the "title" property.
 */
function form_builder_property_title_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['title'] = array(
    '#title' => t('Title'),
    '#type' => 'textfield',
    '#default_value' => $element['#title'],
    '#required' => TRUE,
    '#weight' => -10,
  );

  return $form;
}

/**
 * Configuration form for the "title_display" property.
 */
function form_builder_property_title_display_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['title_display'] = array(
    '#title' => t('Title display'),
    '#type' => 'select',
    '#default_value' => $element['#title_display'],
    '#options' => array(
      'before' => 'before',
      'after' => 'after',
      'invisible' => 'invisible',
      'attribute' => 'attribute',
    ),
    '#required' => TRUE,
    '#weight' => -10,
  );

  return $form;
}


/**
 * Configuration form for the "weight" property.
 *
 * This field is in the "hidden" builder group, meaning it's never shown in
 * the main editing interface. However, it's still there if editing without JS.
 */
function form_builder_property_weight_form(&$form_state, $form_type, $element) {
  $form = array();

  if (!isset($_REQUEST['js'])) {
    $form['weight'] = array(
      '#form_builder' => array('property_group' => 'hidden'),
      '#type' => 'textfield',
      '#size' => 6,
      '#title' => t('Weight'),
      '#default_value' => $element['#weight'],
    );
  }

  return $form;
}

/**
 * Configuration form for the "description" property.
 */
function form_builder_property_description_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $element['#description'],
    '#weight' => 5,
  );

  return $form;
}

/**
 * Configuration form for the "required" property.
 */
function form_builder_property_required_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['required'] = array(
    '#form_builder' => array('property_group' => 'validation'),
    '#title' => t('Required'),
    '#type' => 'checkbox',
    '#default_value' => $element['#required'],
    '#weight' => -1,
  );

  return $form;
}

/**
 * Configuration form for the "options" property.
 */
function form_builder_property_options_form(&$form_state, $form_type, $element) {
  $form = array();

  // Checkboxes have an implied "multiple" property.
  if ($element['#type'] == 'checkboxes') {
    $element['#multiple'] = TRUE;
  }

  $form['options'] = array(
    '#form_builder' => array('property_group' => 'options'),
    '#title' => t('Options'),
    '#type' => 'options',
    '#default_value' => $element['#default_value'],
    '#options' => $element['#options'],
    '#required' => TRUE,
    '#multiple' => isset($element['#multiple']) ? $element['#multiple'] : FALSE,
    '#multiple_toggle' => isset($element['#multiple_toggle']) ? $element['#multiple_toggle'] : FALSE,
    '#optgroups' => $element['#type'] == 'select' ? TRUE : FALSE,
    '#limit' => 100,
    '#key_type' => isset($element['#key_type']) ? $element['#key_type'] : 'mixed',
    '#key_type_toggle' => isset($element['#key_type_toggle']) ? $element['#key_type_toggle'] : TRUE,
    '#key_type_toggled' => isset($element['#key_type_toggled']) ? $element['#key_type_toggled'] : FALSE,
  );

  // Remove the default value field, since it's handled by the options field.
  $form['default_value'] = array();

  return $form;
}

function form_builder_property_options_form_submit(&$form, &$form_state) {
  $options = $form_state['values']['options']['options'];
  $default_value = $form_state['values']['options']['default_value'];

  if (isset($form_state['values']['options']['multiple'])) {
    $multiple = $form_state['values']['options']['multiple'];
    $form_state['values']['multiple'] = $multiple;
  }

  if (isset($form_state['values']['options']['custom_keys'])) {
    $form_state['values']['key_type_toggled'] = $form_state['values']['options']['custom_keys'];
  }

  // Select elements with no default value sometimes want a "None" option to
  // automatically be added to the list, which we achieve by adding the
  // #empty_value property to the element. This is an opt-in feature; it will
  // only take effect if 'empty_value' is also declared as an allowed property
  // in the element's hook_form_builder_types() implementation.
  if ($form['#_edit_element']['#type'] == 'select' && empty($form['#_edit_element']['#multiple']) && !isset($default_value)) {
    // This would normally be set to an empty string, but if we did that,
    // form_builder_field_configure_submit() would remove the property and it
    // would not take effect. Since this is only used for display purposes in
    // the form builder editing interface, any value works as long as it
    // doesn't correspond to one of the #options, so we choose one that is very
    // unlikely to conflict.
    $form_state['values']['empty_value'] = 'form-builder-empty-value';
  }

  $form_state['values']['options'] = $options;
  $form_state['values']['default_value'] = $default_value;
}

/**
 * Configuration form for the "default_value" property.
 */
function form_builder_property_default_value_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['default_value'] = array(
    // Most fields have default values that can be stored on one line, so we
    // use a textfield to configure their default value. Textareas are an
    // exception, though.
    '#type' => $element['#type'] == 'textarea' ? 'textarea' : 'textfield',
    '#title' => t('Default value'),
    '#default_value' => $element['#default_value'],
    '#weight' => 1,
  );

  return $form;
}

/**
 * Configuration form for the "markup" property.
 */
function form_builder_property_markup_form(&$form_state, $form_type, $element) {
  $form = array();

  // TODO: This is a placeholder until "#markup" becomes available in D7.
  $form['markup'] = array(
    '#type' => 'text_format',
    '#title' => t('Markup'),
    '#default_value' => $element['#markup'],
    '#format' => isset($element['#format']) ? $element['#format'] : NULL,
    '#weight' => 1,
  );

  return $form;
}

/**
 * Submit handler for the "markup" property.
 */
function form_builder_property_markup_form_submit(&$form, &$form_state) {
  // The submitted data from an element of type 'text_format' is an array that
  // contains the text and its format in separate keys. We want these to wind
  // up in #markup and #format, respectively.
  $markup_array = $form_state['values']['markup'];
  $form_state['values']['markup'] = $markup_array['value'];
  $form_state['values']['format'] = $markup_array['format'];
}

/**
 * Configuration form for the "size" property.
 */
function form_builder_property_size_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['size'] = array(
    '#form_builder' => array('property_group' => 'display'),
    '#type' => 'textfield',
    '#size' => 6,
    '#title' => t('Size'),
    '#default_value' => $element['#size'],
    '#weight' => 2,
  );

  return $form;
}

/**
 * Configuration form for the "rows" property.
 */
function form_builder_property_rows_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['rows'] = array(
    '#form_builder' => array('property_group' => 'display'),
    '#type' => 'textfield',
    '#size' => 6,
    '#title' => t('Rows'),
    '#default_value' => $element['#rows'],
    '#weight' => 2,
  );

  return $form;
}

/**
 * Configuration form for the "cols" property.
 */
function form_builder_property_cols_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['cols'] = array(
    '#form_builder' => array('property_group' => 'display'),
    '#type' => 'textfield',
    '#size' => 6,
    '#title' => t('Columns'),
    '#default_value' => $element['#cols'],
    '#weight' => 3,
    '#description' => t('The width of the textarea. This property might not have a visual impact depending on the CSS of your site.'),
  );

  return $form;
}


/**
 * Configuration form for the "field_prefix" property.
 */
function form_builder_property_field_prefix_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['field_prefix'] = array(
    '#form_builder' => array('property_group' => 'display'),
    '#type' => 'textfield',
    '#title' => t('Prefix'),
    '#default_value' => $element['#field_prefix'],
    '#weight' => -2,
  );

  return $form;
}

/**
 * Configuration form for the "field_suffix" property.
 */
function form_builder_property_field_suffix_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['field_suffix'] = array(
    '#form_builder' => array('property_group' => 'display'),
    '#type' => 'textfield',
    '#title' => t('Suffix'),
    '#default_value' => $element['#field_suffix'],
    '#weight' => -1,
  );

  return $form;
}

/**
 * Configuration form for the "collapsible" property.
 */
function form_builder_property_collapsible_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['collapsible'] = array(
    '#form_builder' => array('property_group' => 'display'),
    '#type' => 'checkbox',
    '#title' => t('Collapsible'),
    '#default_value' => $element['#collapsible'],
    '#weight' => -2,
  );

  return $form;
}

/**
 * Configuration form for the "collapsed" property.
 */
function form_builder_property_collapsed_form(&$form_state, $form_type, $element) {
  $form = array();

  $form['collapsed'] = array(
    '#form_builder' => array('property_group' => 'display'),
    '#type' => 'checkbox',
    '#title' => t('Collapsed'),
    '#default_value' => $element['#collapsed'],
    '#weight' => -1,
    '#description' => t('This property will not affect the preview immediately.'),
  );

  return $form;
}

<?php
// $Id: form_builder.admin.inc,v 1.20 2011/01/12 04:05:28 quicksketch Exp $

/**
 * @file form_builder.admin.inc
 * Administrative interface for editing forms.
 */

/**
 * Main form building interface. Can be used as a menu callback.
 *
 * @param $form_type
 *   The type of form being edited. Usually the name of the providing module.
 * @param $form_id
 *   The unique identifier for the form being edited with the type.
 */
function form_builder_interface($form_type, $form_id) {
  module_load_include('inc', 'form_builder', 'includes/form_builder.api');
  module_load_include('inc', 'form_builder', 'includes/form_builder.cache');

  // Set the current form type (used for display of the sidebar block).
  form_builder_active_form($form_type, $form_id);

  // Load the current state of the form, or create a new cache if needed.
  $form_structure = form_builder_cache_load($form_type, $form_id);
  if (!$form_structure) {
    $form_structure = form_builder_load_form($form_type, $form_id);
    form_builder_cache_save($form_type, $form_id, $form_structure);
  }

  $output = array();
  $output[] = drupal_get_form('form_builder_preview', $form_structure, $form_type, $form_id);
  $output[] = drupal_get_form('form_builder_positions', $form_structure, $form_type, $form_id);

  return $output;
}

/**
 * Menu callback for adding a field.
 */
function form_builder_add_page($form_type, $form_id, $element_type) {
  module_load_include('inc', 'form_builder', 'includes/form_builder.api');
  module_load_include('inc', 'form_builder', 'includes/form_builder.cache');

  $fields = form_builder_get_form_type($form_type);
  if (isset($fields[$element_type])) {
    $cache = form_builder_cache_load($form_type, $form_id);
    $element_id = isset($_REQUEST['element_id']) ? $_REQUEST['element_id'] : 'new_' . time();
    $element = $fields[$element_type]['default'];

    // Set the element ID to a hard-coded value if a unique field type.
    if (isset($fields[$element_type]['unique']) && $fields[$element_type]['unique']) {
      $element_id = $element_type;
    }

    $element['#form_builder']['element_id'] = $element_id;
    $element['#form_builder']['is_new'] = TRUE;
    $element = form_builder_add_default_properties($element, $form_type, $element_id);

    $element['#weight'] = count(element_children($cache));

    drupal_alter('form_builder_add_element', $element, $form_type, $form_id);

    // Save any element ID set by the hook_form_builder_add_element_alter().
    $element_id = $element['#form_builder']['element_id'];

    form_builder_cache_field_save($form_type, $form_id, $element);

    if (isset($_REQUEST['js'])) {
      $element = form_builder_cache_field_load($form_type, $form_id, $element_id);
      $preview_form = form_builder_cache_load($form_type, $form_id);
      $data = array(
        'formType' => $form_type,
        'formId' => $form_id,
        'elementId' => $element_id,
        'html' => form_builder_field_render($form_type, $form_id, $element_id),
        'positionForm' => drupal_render(drupal_get_form('form_builder_positions', $preview_form, $form_type, $form_id)),
      );

      form_builder_render_json($data);
      exit();
    }
  }

  // Otherwise return to the previous page.
  drupal_goto();
}

/**
 * Menu callback for configuring a field.
 */
function form_builder_configure_page($form_type, $form_id, $element_id) {
  $output = drupal_get_form('form_builder_field_configure', $form_type, $form_id, $element_id);

  if (isset($_REQUEST['js'])) {
    // Return the newly changed field.
    if (isset($_REQUEST['return'])) {
      form_builder_field_json($form_type, $form_id, $element_id);
    }
    // Display the configuration form for a field.
    else {
      $data = array(
        'formType' => $form_type,
        'formId' => $form_id,
        'elementId' => $element_id,
        'html' => drupal_render($output),
        'errors' => form_get_errors(),
      );
      form_builder_render_json($data);
      exit();
    }
  }

  return $output;
}

/**
 * Menu callback for removing a field.
 */
function form_builder_remove_page($form_type, $form_id, $element_id) {
  $output = drupal_get_form('form_builder_field_remove', $form_type, $form_id, $element_id);

  if (isset($_REQUEST['js']) && !isset($_REQUEST['return'])) {
    // This after build function immediately returns the form as JSON.
    $data = array(
      'formType' => $form_type,
      'formId' => $form_id,
      'elementId' => $element_id,
      'html' => drupal_render($output),
    );

    form_builder_render_json($data);
    exit();
  }

  return $output;
}

/**
 * Form. Given a form array, present it for editing in a preview.
 */
function form_builder_preview($f, &$form_state, $form, $form_type, $form_id) {

  // @todo: think about this more. We basically get the form from outside, so
  // while normally the first argument would be $form, we use the third, which
  // is provided by the caller.

  // Make modifications to all form elements recursively.
  $element_ids = form_builder_preview_prepare($form, $form_type, $form_id);

  // Record all the element IDs within the entire form.
  $form['#form_builder']['element_ids'] = $element_ids;
  $form['#form_builder']['form_type'] = $form_type;
  $form['#form_builder']['form_id'] = $form_id;

  // Add a pre_render to the entire form itself.
  $form['#pre_render'][] = 'form_builder_pre_render_form';
  $form['#theme_wrappers'] = array('form_builder_wrapper');

  // Add required jQuery UI elements.
  $form['#attached']['library'][] = array('system', 'ui.draggable');
  $form['#attached']['library'][] = array('system', 'ui.droppable');
  $form['#attached']['library'][] = array('system', 'ui.sortable');

  $form['#attached']['library'][] = array('system', 'form');
  $form['#attached']['js'][] = drupal_get_path('module', 'form_builder') .'/form_builder.js';

  // TODO: This JS file should be loaded dynamically as needed.
  $form['#attached']['js'][] = drupal_get_path('module', 'options_element') .'/options_element.js';
  // @todo: no library for this? see http://drupal.org/node/954804
  $form['#attached']['js'][] = 'misc/tabledrag.js';
  $form['#attached']['js'][] = 'misc/jquery.cookie.js';
  $form['#attached']['library'][] = array('system', 'form');
  // @todo: no library for this?
  $form['#attached']['js'][] = 'misc/collapse.js';
  $form['#attached']['js'][] = array('data' => array('formBuilder' => array('emptyFieldset' => theme('form_builder_empty_fieldset'))), 'type' => 'setting');

  $form['#attached']['css'][] = drupal_get_path('module', 'form_builder') .'/form_builder.css';
  $form['#attached']['css'][] = drupal_get_path('module', 'options_element') .'/options_element.css';

  return $form;
}

/**
 * Form containing all the current weights and parents of elements.
 */
function form_builder_positions($form, &$form_state, $preview_form, $form_type, $form_id) {
  $form = array(
    '#tree' => TRUE,
    '#form_builder' => array(
      'form_type' => $form_type,
      'form_id' => $form_id,
      'form' => $preview_form,
    ),
  );

  form_builder_positions_prepare($form, $preview_form);

  // Drupal MUST have a button to register submissions.
  // Add a button even though the form is only submitted via AJAX.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  return $form;
}

/**
 * Recursive helper for form_builder_positions(). Add weight fields.
 */
function form_builder_positions_prepare(&$form, &$preview_form, $parent_id = FORM_BUILDER_ROOT) {
  foreach (element_children($preview_form) as $key) {
    // Keep record of the current parent ID.
    $previous_parent_id = $parent_id;

    if (isset($preview_form[$key]['#form_builder']['element_id'])) {
      // Set the parent ID for this element.
      $preview_form[$key]['#form_builder']['parent_id'] = $parent_id;
      $element_id = $preview_form[$key]['#form_builder']['element_id'];
      $parent_id = $element_id;

      $form[$element_id]['weight'] = array(
        '#type' => 'hidden',
        '#default_value' => isset($preview_form[$key]['#weight']) ? $preview_form[$key]['#weight'] : 0,
        '#attributes' => array('class' => array('form-builder-weight form-builder-element-' . $element_id)),
      );
      $form[$element_id]['parent'] = array(
        '#type' => 'hidden',
        '#default_value' => $preview_form[$key]['#form_builder']['parent_id'],
        '#attributes' => array('class' => array('form-builder-parent form-builder-element-' . $element_id)),
      );
    }

    form_builder_positions_prepare($form, $preview_form[$key], $parent_id);
    $parent_id = $previous_parent_id;
  }
}

/**
 * Submit handler for the form_builder_positions form.
 */
function form_builder_positions_submit(&$form, &$form_state) {
  module_load_include('inc', 'form_builder', 'includes/form_builder.api');
  module_load_include('inc', 'form_builder', 'includes/form_builder.cache');

  $form_type = $form['#form_builder']['form_type'];
  $form_id = $form['#form_builder']['form_id'];
  $preview_form = $form['#form_builder']['form'];

  foreach (element_children($form) as $element_id) {
    // Skip items without weight value (like the form token, build_id, etc).
    if (!isset($form[$element_id]['weight'])) {
      continue;
    }

    // Check for changed weights or parents.
    $element = form_builder_get_element($preview_form, $element_id);
    $element['#weight'] = $form_state['values'][$element_id]['weight'];
    $element['#form_builder']['parent_id'] = $form_state['values'][$element_id]['parent'];
    form_builder_set_element($preview_form, $element);
  }

  // Save all the changes made.
  form_builder_cache_save($form_type, $form_id, $preview_form);
}

/**
 * Adds a wrapper around the form_builder preview and optionally will add the
 * field palette if it is not already available as a block.
 */
function theme_form_builder_wrapper($vars) {
  $element = $vars['element'];

  // Open the wrapper.
  $output = '<div id="form-builder-wrapper" class="' . ($element['#block_enabled'] ? 'no-palette' : 'with-palette') . '">';

  // If we need to add the field palette add it here.
  if (!$element['#block_enabled']) {
    $fields = form_builder_get_form_type($element['#form_builder']['form_type']);
    $groups = module_invoke_all('form_builder_palette_groups');
    $output .= '<div id="form-builder-fields">';
    $output .= '<div class="block">';
    $output .= theme('form_builder_field_palette', array('fields' => $fields, 'groups' => $groups, 'form_type' => $element['#form_builder']['form_type'], 'form_id' => $element['#form_builder']['form_id']));
    $output .= '</div>';
    $output .= '</div>';
  }

  $output .= '<div id="form-builder">';
  if (isset($element['#title'])) {
    $output .= '<h3>' . $element['#title'] . '</h3>';
  }

  // Add the contents of the form and close the wrappers.
  $output .= $element['#children'];
  $output .= '</div></div>';

  return $output;
}

/**
 * Theme the wrapper around a form_builder element and add configure and remove
 * links as appropriate.
 */
function theme_form_builder_element_wrapper($vars) {
  $element = $vars['element'];

  $removable = isset($element['#form_builder']['removable']) ? $element['#form_builder']['removable'] : TRUE;
  $configurable  = isset($element['#form_builder']['configurable']) ? $element['#form_builder']['configurable'] : TRUE;

  $output = '<div class="form-builder-wrapper">';

  if ($removable || $configurable) {
    $output .= '<div class="form-builder-title-bar"><span class="form-builder-links">';
    if ($removable) {
      $output .= l('<span class="remove">'. t('Remove') .'</span>', 'admin/build/form-builder/remove/' . $element['#form_builder']['form_type'] . '/' . $element['#form_builder']['form_id'] . '/' . $element['#form_builder']['element_id'], array('html' => TRUE, 'attributes' => array('class' => array('remove'), 'title' => t('Remove')), 'query' => drupal_get_destination()));
    }
    if ($removable && $configurable) {
      $output .= ' ';
    }
    if ($configurable) {
      $output .= l('<span class="configure">'. t('Configure') .'</span>', 'admin/build/form-builder/configure/' . $element['#form_builder']['form_type'] . '/' . $element['#form_builder']['form_id'] . '/' . $element['#form_builder']['element_id'], array('html' => TRUE, 'attributes' => array('class' => array('configure'), 'title' => t('Configure')), 'query' => drupal_get_destination()));
    }
    $output .= '</span></div>';
  }

  $output .= '<div class="form-builder-element form-builder-element-' . $element['#type'] . '" id="form-builder-element-' . $element['#form_builder']['element_id'] . '">';
  // TODO: Overlay image: good idea or bad idea? Prevents any interaction with
  // form elements in the preview.
  //$output .= theme('image', drupal_get_path('module', 'form_builder') .'/images/blank.gif', '', '', array('width' => '1', 'height' => '1', 'class' => 'form-builder-disable'));
  $output .= $element['#children'];

  $output .= '</div></div>';

  return $output;
}

/**
 * Placeholder for empty fieldsets during drag and drop.
 */
function theme_form_builder_empty_fieldset($vars) {
  $output = '';
  $output .= '<div class="form-builder-wrapper form-builder-empty-placeholder">';
  $output .= '<span>' . t('This fieldset is empty. Drag a form element into it.') .'</span>';
  $output .= '</div>';
  return $output;
}

/**
 * Block for adding new fields.
 *
 * @param $vars['fields']
 *   A list of all fields can be added to the current form type.
 * @param $vars['groups']
 *   A list of groups that fields may be sorted into. Each field is assigned
 *   a 'palette_group' property which corresponds to one of these groups.
 * @param $vars['form_type']
 *   The form type to which these blocks apply.
 * @param $vars['form_id']
 *   The form ID that is being edited.
 */
function theme_form_builder_field_palette($vars) {
  extract($vars);

  $output = '';
  $lists = array();
  foreach ($fields as $field_name => $field) {
    $class = array('field-' . $field_name);
    $style = '';

    // If a field is unique, add a special class to identify it.
    if ($field['unique']) {
      $class[] = 'form-builder-unique';
      $class[] = 'form-builder-element-' . $field_name;

      // If already in use, do not display it in the list.
      if (!empty($field['in_use'])) {
        $style = 'display: none;';
      }
    }

    $lists[$field['palette_group']]['#weight'] = $groups[$field['palette_group']]['weight'];
    $lists[$field['palette_group']][] = array(
      'data' => l($field['title'], 'admin/build/form-builder/add/' . $form_type . '/' . $form_id . '/' . $field_name, array('query' => drupal_get_destination())),
      'class' => $class,
      'style' => $style,
    );
  }

  // Sort the lists by weight.
  uasort($lists, 'element_sort');

  $output .= '<div id="form-builder-field-palette">';
  foreach ($lists as $group => $list) {
    unset($list['#weight']);
    $output .= theme('item_list', array('items' => $list, 'title' => (count($lists) > 1) ? $groups[$group]['title'] : t('Add a field'), 'type' => 'ul', 'attributes' => array('class' => array('form-builder-fields', 'clearfix'))));
  }
  $output .= '</div>';

  return $output;
}

/**
 * Take a form structure and add a prebuild function to every element.
 */
function form_builder_pre_render($element) {
  $element['#theme_wrappers'][] = 'form_builder_element_wrapper';

  if ($element['#form_builder']['element_type'] == 'fieldset') {
    $element['#attributes']['class'][] = 'form-builder-fieldset';
  }

  if (isset($element['#type']) && $element['#type'] == 'fieldset' && count(element_children($element)) == 0) {
    $element['#children'] = theme('form_builder_empty_fieldset');
  }

  // Allow modules to make modifications to this element.
  drupal_alter('form_builder_preview', $element, $element['#form_builder']['form_type'], $element['#form_builder']['form_id']);

  return $element;
}

/**
 * This changes the type to markup before rendering.  Also sets a default title
 * and sets a variable to indicate to the theme wrapper that the field palette
 * should be added.
 */
function form_builder_pre_render_form($form) {
  // We can't have forms inside of forms, so change this entire form a markup.
  $form['#type'] = 'markup';

  // Remove unnecessary FAPI elements.
  unset($form['form_build_id']);
  unset($form['form_token']);
  unset($form['form_builder_preview']);

  global $theme;
  // Check if the Form Builder block is enabled.
  // Otherwise make our own columns.
  $form['#block_enabled'] = db_query("SELECT status FROM {block} WHERE module = 'form_builder' AND theme = :theme", array(':theme' => $theme))->fetchField();

  if ($block_enabled && ($theme == 'garland' || $theme == 'minnelli')) {
    $form['#attached']['css'][] = drupal_get_path('module', 'form_builder') .'/form_builder.garland.css';
  }

  $form['#title'] = t('Form preview');

  return $form;
}

function form_builder_after_build($element) {
  $element['#attributes']['readonly'] = 'readonly';
  foreach (element_children($element) as $key) {
    $element[$key] = form_builder_after_build($element[$key]);
  }

  return $element;
}

/**
 * Before editing a form, modify it slightly to add functionality used in
 * the preview and disable use of the actual form fields in any way.
 *
 * @return
 *   A list of all element_ids currently used within this form.
 */
function form_builder_preview_prepare(&$form, $form_type, $form_id, $parent_id = FORM_BUILDER_ROOT) {
  $element_ids = array();
  foreach (element_children($form) as $key) {
    // Keep record of the current parent ID.
    $previous_parent_id = $parent_id;

    if (isset($form[$key]['#form_builder']['element_id'])) {
      $element_ids[] = $form[$key]['#form_builder']['element_id'];
      $form[$key]['#pre_render'][] = 'form_builder_pre_render';
      $form[$key]['#form_builder']['form_type'] = $form_type;
      $form[$key]['#form_builder']['form_id'] = $form_id;
      $form[$key]['#form_builder']['parent_id'] = $parent_id;
      // Do not allow the #pre_render function we added above to force any
      // #pre_render functions associated with the element itself to be
      // skipped; we still want those to run whenever they otherwise would
      // have.
      // @todo: Revisit if there is a different way to do this after
      //   http://drupal.org/node/914792 is committed to core.
      if (count($form[$key]['#pre_render']) == 1) {
        $info = element_info($form[$key]['#type']);
        if (!empty($info['#pre_render'])) {
          $form[$key]['#pre_render'] = array_merge($form[$key]['#pre_render'], $info['#pre_render']);
        }
      }

      $parent_id = $form[$key]['#form_builder']['element_id'];
    }

    // Search within this element for further form elements.
    $additional_ids = form_builder_preview_prepare($form[$key], $form_type, $form_id, $parent_id);
    $element_ids = array_merge($element_ids, $additional_ids);
    $parent_id = $previous_parent_id;
  }

  return $element_ids;
}

/**
 * Form for editing a field.
 */
function form_builder_field_configure($form, $form_state, $form_type, $form_id, $element_id) {
  module_load_include('inc', 'form_builder', 'includes/form_builder.api');
  module_load_include('inc', 'form_builder', 'includes/form_builder.cache');
  $element = form_builder_cache_field_load($form_type, $form_id, $element_id);

  // Assemble a form made up of all the configurable properties that this type
  // of form supports.
  $form = array();
  foreach (form_builder_get_element_properties($form_type, $element['#form_builder']['element_type']) as $property => $property_settings) {
    if (isset($property_settings['form']) && function_exists($property_settings['form'])) {
      $function = $property_settings['form'];
      // Set a default value on the property to avoid notices.
      $element['#' . $property] = isset($element['#' . $property]) ? $element['#' . $property] : NULL;
      $form = array_merge($form, $function($form_state, $form_type, $element));
    }
  }

  $form['#_edit_form_type'] = $form_type;
  $form['#_edit_form_id'] = $form_id;
  $form['#_edit_element_id'] = $element_id;
  $form['#_edit_element'] = $element;

  $form['form_builder_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
    '#weight' => 100,
  );

  $form['#pre_render'][] = 'form_builder_pre_render_field_configure';

  return $form;
}

function form_builder_pre_render_field_configure($form) {
  $groups = module_invoke_all('form_builder_property_groups', $form['#form_type']);

  foreach (element_children($form) as $key) {
    // If no group is specified, put the element into the default group.
    if (!isset($form[$key]['#form_builder']['property_group']) || !isset($groups[$form[$key]['#form_builder']['property_group']])) {
      if (!isset($form[$key]['#type']) || (isset($form[$key]['#type']) && !in_array($form[$key]['#type'], array('hidden', 'button', 'submit', 'value', 'token')))) {
        $form[$key]['#form_builder']['property_group'] = 'default';
      }
    }

    if (isset($form[$key]['#form_builder']['property_group'])) {
      $group = $form[$key]['#form_builder']['property_group'];

      // We add "_property_group" to the field key to prevent conflicts of
      // property names and group names.
      if (!isset($form[$group . '_property_group'])) {
        $form[$group . '_property_group'] = array(
          '#type' => 'fieldset',
          '#title' => $groups[$group]['title'],
          '#weight' => $groups[$group]['weight'],
          '#collapsible' => isset($groups[$group]['collapsible']) ? $groups[$group]['collapsible'] : FALSE,
          '#collapsed' => isset($groups[$group]['collapsed']) ? $groups[$group]['collapsed'] : FALSE,
          '#attributes' => array('class' => array('form-builder-group')),
        );
      }

      $form[$group .'_property_group'][$key] = $form[$key];
      unset($form[$key]);
    }
  }

  return $form;
}

function form_builder_field_configure_submit(&$form, &$form_state) {
  $form_type = $form['#_edit_form_type'];
  $form_id = $form['#_edit_form_id'];
  $element_id = $form['#_edit_element_id'];
  $element = $form['#_edit_element'];

  // Allow each element to do any necessary submission handling.
  foreach (form_builder_get_element_properties($form_type, $element['#form_builder']['element_type']) as $property => $property_settings) {
    if (isset($property_settings['submit'])) {
      foreach ($property_settings['submit'] as $function) {
        if (function_exists($function)) {
          $function($form, $form_state);
        }
      }
    }
  }

  // Allow the element to be updated in a hard-coded fashion by altering the
  // $form['#element'] item. Using this approach skips the property check.
  $element = $form['#_edit_element'];

  // Update the field according to the settings in $form_state['values'].
  $saveable = form_builder_get_saveable_properties($form_type, $element);
  foreach ($form_state['values'] as $property => $value) {
    if (in_array($property, $saveable)) {
      // Remove empty properties entirely.
      if ($value == '') {
        unset($element['#'. $property]);
      }
      else {
        $element['#'. $property] = $value;
      }
    }
  }

  // Update the form builder cache.
  form_builder_cache_field_save($form_type, $form_id, $element);

  if (isset($_GET['js'])) {
    // Option A: Use the destination variable to do a drupal_goto(). Allows
    // other submit handlers to add on extra functionality.
    // The destination variable takes precedence over $form_state['redirect'].
    //$_REQUEST['destination'] = 'admin/build/form-builder/json/' . $form_type . '/' . $form_id . '/' . $element_id;

    // Option B: Immediately print the JSON and exit. Faster and requires only
    // one HTTP request instead of two. Other submit handlers must be before
    // this on.
    form_builder_field_json($form_type, $form_id, $element_id);
  }
}

/**
 * Form for removing a field.
 */
function form_builder_field_remove($form, $form_state, $form_type, $form_id, $element_id) {
  module_load_include('inc', 'form_builder', 'includes/form_builder.api');
  module_load_include('inc', 'form_builder', 'includes/form_builder.cache');
  $element = form_builder_cache_field_load($form_type, $form_id, $element_id);

  $form = array();
  $form['#_edit_form_type'] = $form_type;
  $form['#_edit_form_id'] = $form_id;
  $form['#_edit_element_id'] = $element_id;

  $question = t('Remove the field %title?', array('%title' => $element['#title']));
  $path = isset($_GET['destination']) ? $_GET['destination'] : NULL;
  $description = t('Remove the field %title? This field will not be permanently removed until the form configuration is saved.', array('%title' => isset($element['#title']) ? $element['#title'] : $element['#form_builder']['element_id']));
  $yes = t('Remove');

  return confirm_form($form, $question, $path, $description, $yes);
}

function form_builder_field_remove_submit(&$form, &$form_state) {
  $form_type = $form['#_edit_form_type'];
  $form_id = $form['#_edit_form_id'];
  $element_id = $form['#_edit_element_id'];

  // Update the form builder cache.
  form_builder_cache_field_delete($form_type, $form_id, $element_id);

  if (isset($_GET['js'])) {
    // See form_builder_field_configure_submit() for comparison between using
    // redirect and immediately printing the JSON.
    //$form_state['redirect'] = 'admin/build/form-builder/json/' . $form_type . '/' . $form_id . '/' . $element_id;
    form_builder_field_json($form_type, $form_id, $element_id);
  }
}

/**
 * Render a single field independent of other settings.
 */
function form_builder_field_render($form_type, $form_id, $element_id, $wrapper = FALSE) {
  module_load_include('inc', 'form_builder', 'includes/form_builder.api');
  module_load_include('inc', 'form_builder', 'includes/form_builder.cache');

  // Load the current state of the form and prepare it for rendering.
  $form = form_builder_cache_load($form_type, $form_id);
  if (!$form) {
    $form = form_builder_load_form($form_type, $form_id);
    form_builder_cache_save($form_type, $form_id, $form);
  }
  $form_state = array('submitted' => FALSE, 'build_info' => array('args' => array($form, $form_type, $form_id)), 'values' => array(), 'method' => 'get');
  $form = drupal_retrieve_form('form_builder_preview', $form_state);
  drupal_prepare_form('form_builder_preview', $form, $form_state);
  $form['#post'] = array();
  $form = form_builder('form_builder_preview', $form, $form_state);

  // Get only the element wanted and render it.
  $element = form_builder_get_element($form, $element_id);

  if ($wrapper) {
    $element['#theme_wrappers'][] = 'form_builder_element_wrapper';
  }

  return drupal_render($element);
}

/**
 * Menu callback to display a field as a JSON string.
 */
function form_builder_field_json($form_type, $form_id, $element_id) {
  module_load_include('inc', 'form_builder', 'includes/form_builder.api');
  module_load_include('inc', 'form_builder', 'includes/form_builder.cache');
  $element = form_builder_cache_field_load($form_type, $form_id, $element_id);

  $data = array(
    'formType' => $form_type,
    'formId' => $form_id,
    'elementId' => $element_id,
    'html' => form_builder_field_render($form_type, $form_id, $element_id),
    'errors' => form_get_errors(),
  );

  form_builder_render_json($data);
  exit();
}

/**
 * Adds status messages, settings, and timestamp to a form builder JSON response and outputs it.
 */
function form_builder_render_json($data) {
  if (!isset($data['messages'])) {
    $data['messages'] = theme('status_messages');
  }
  if (!isset($data['settings'])) {
    $scripts = drupal_add_js();
    if (!empty($scripts['settings'])) {
      $data['settings'] = drupal_array_merge_deep_array($scripts['settings']['data']);
    }
  }
  if (!isset($data['time'])) {
    $data['time'] = time();
  }
  drupal_json_output($data);
}

/*jslint bitwise: true, eqeqeq: true, immed: true, newcap: true, nomen: false,
 onevar: false, plusplus: false, regexp: true, undef: true, white: true, indent: 2
 browser: true */

/*global jQuery: true Drupal: true window: true ThemeBuilder: true */

(function ($) {

  Drupal.seoUI = Drupal.seoUI || {};

  Drupal.behaviors.seoUI = {
    attach: function (context, settings) {
      // Set the summary of the node edit form vertical tab
      $('fieldset.seo-settings', context).drupalSetSummary(function (context) {
        var output = [];
        // Path and Pathauto
        var path = $('.form-item-path-alias input').val();
        var automatic = $('.form-item-path-pathauto input').attr('checked');

        if (automatic) {
          output.push(Drupal.t('Automatic alias'));
        }
        else if (path) {
          output.push(Drupal.t('URL alias: @alias', {'@alias': path}));
        }
        else {
          output.push(Drupal.t('No alias'));
        }

        // Redirect
        if ($('table.redirect-list', context).size()) {
          if ($('table.redirect-list tbody td.empty', context).size()) {
            output.push(Drupal.t('No redirects'));
          }
          else {
            var redirects = $('table.redirect-list tbody tr').size();
            output.push(Drupal.formatPlural(redirects, '1 redirect', '@count redirects'));
          }
        }
        // Join the various vertical tab messages with a break tag in between them.
        return output.join('<br />');
      });
    }
  };
})(jQuery);

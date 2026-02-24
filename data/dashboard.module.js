'use strict';

var app = angular.module('dashboardApp', [
  'ngRoute',
  'ui.bootstrap',
  'xeditable',
  'targetsList',
  'triggersList',
  'peripheralsList',
  'network',
  'checklist-model'
]);

app.run(['editableOptions', function(editableOptions) {
  editableOptions.theme = 'bs4'; // bootstrap3 theme. Can be also 'bs4', 'bs2', 'default'
}]);
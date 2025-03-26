'use strict';

angular.
  module('dashboardApp').
  config(['$routeProvider',
    function config($routeProvider) {
       $routeProvider
          .when("/", {
            templateUrl : "<network></network>"
          })
          .when("/network", {
            template: '<network></network>', 
          })
          .when("/triggers", {
            template: '<triggers-list></triggers-list>', 
          })
          .when("/peripherals", {
            template: '<peripherals-list></peripherals-list>', 
          })
          .when("/mesh", {
            templateUrl : "mesh.html",
            controller: "meshCtrl"
          })
          .when("/admin", {
            templateUrl : "admin.html"
        })
    }
  ]);
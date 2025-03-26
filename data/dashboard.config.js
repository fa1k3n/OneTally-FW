'use strict';

angular.
  module('dashboardApp').
  config(['$routeProvider',
    function config($routeProvider) {
       $routeProvider
          .when("/", {
            templateUrl : "admin.html"
          })
          .when("/triggers", {
            template: '<triggers-list></triggers-list>', 
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
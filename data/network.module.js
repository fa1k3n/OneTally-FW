'use strict';

angular.module('network', ['ui.bootstrap']);
angular.module('network').
    component('network', {
        templateUrl: 'network.templ.html',
        controller: 'networkCtrl'
    })

angular.module('network').
    controller('networkCtrl', function($scope, $http) {
        
    $scope.data = {}

    $http.get("/network")
        .then(function(response) {                
            $scope.data = response.data
        });
       
    })

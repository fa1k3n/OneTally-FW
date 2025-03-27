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

    $scope.switchers = [
        { type: "gostream", name: "GoStream" },
        { type: "obs", name: "OBS" }
    ]
        
    $http.get("/network")
        .then(function(response) {                
            $scope.data = response.data
        });
       
    $scope.commit = function() {
        $http.post("/network", $scope.data).then(function(resp) {
            $http.post("/commit").then(function(response) {
                showSuccessMessage("Saved network settings succesfully")
            })
        })
    }
    })

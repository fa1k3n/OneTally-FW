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
       
    $scope.save = function() {
        $http.put("/network", $scope.data).then(function(resp) {
            $http.put("/commit", {}).then(function(response) {
                showSuccessMessage("Network settings updated successfully");
            })
            
            $http.get("/network")
            .then(function(response) {                
                $scope.data = response.data
            });
            //$http.post("/commit").then(function(response) {
            //    showSuccessMessage("Saved network settings succesfully")
            //})
        })
    }
    })

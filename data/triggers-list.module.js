'use strict';

angular.module('triggersList', [
  'triggerDetails',
  'ui.bootstrap'
]);

angular.module('triggersList').
  component('triggersList', {
    templateUrl: 'triggers-list.template.html',
    controller: 'triggersListCtrl'
   }) 
   
angular.module('triggersList').
  controller('triggersListCtrl', 
    function($scope, $http, $filter) {
        $scope.data = []

        $scope.peripherals = []

        $scope.events = {
            "1": [   // Internal
                {"id": 4, "name": "configuration"},
            ],
            "2": [   // GoStream D8
                { "id": 1, "name": "onPgm" },
                { "id": 2, "name": "onPvw" },
                { "id": 3, "name": "connecting" },
                { "id": 4, "name": "connected" },
            ]
        }

        $scope.srcIds = [
            { "id": 1, "name": "In 1" },
            { "id": 2, "name": "In 2" },
            { "id": 3, "name": "In 3" },
            { "id": 4, "name": "In 4" },
            { "id": 5, "name": "In 5" },
            { "id": 6, "name": "In 6" },
            { "id": 7, "name": "In 7" },
            { "id": 8, "name": "In 8" },
        ]
        
        $scope.targets = []

        $scope.findTargetById = function(targetId) {
            var selected = [];
            if(targetId) {
                selected = $filter('filter')($scope.targets, {id: targetId});
            }
            return selected.length ? selected[0].name : 'Not set';
        }

        $scope.pifIdsToNames = function(pifIds) {
            var selected = [];
            angular.forEach($scope.peripherals, function(s) {
                if (pifIds.indexOf(s.id) >= 0) {
                    selected.push(s.name);
                }
            });
            return selected.length ? selected.join(', ') : 'Not set';
        }

        $scope.getEventList = function(trigger) {
            var selected = [];
            if(trigger) {
                selected = $filter('filter')($scope.events[trigger.target], {id: trigger.event});
            }
            return selected.length ? selected[0].name : 'Not set';
        }

        $http.get("/peripherals")
            .then(function(response) {                
                $scope.peripherals = response.data
            });
        $http.get("/targets")
            .then(function(response) {                
                $scope.targets = response.data
            });

        $http.get("/triggers")
            .then(function(response) {                
                $scope.data = response.data
            });
            
        $scope.brightnessChange = function(trigger) {
            $http.put("/triggers/" + trigger.id + "/brightness", trigger).then(function(response) {})
        }

        $scope.saveTrigger = function(trigger) {
             $http.put("/triggers/" + trigger.id, trigger).then(function(response) {
              $http.put("/commit", {}).then(function(response) {
                showSuccessMessage("Trigger updated successfully")
              })

              $http.get("/triggers/" + trigger.id)
                .then(function(response) {                
                  let index = $scope.data.findIndex((item) => item.id === trigger.id)  
                  $scope.data[index] = response.data
                });
          });
        }

        $scope.commit = function() {
            $http.put("/commit", {}).then(function(response) {
                showSuccessMessage("Trigger updated successfully")
            })
        }

        $scope.new = function(parentSelector) {}

       $scope.delete = function(id) {
          $http.delete("/triggers/" + id).then(function(response) {
            showSuccessMessage(id + " deleted successfully")
            $http.get("/triggers")
              .then(function(response) {                
                $scope.data = response.data
              });
          }, function(resp) {
            alert ("Failed to delete " + resp)
          })
        }
    })
    
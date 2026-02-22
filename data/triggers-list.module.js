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
    function($scope, $http, $uibModal, $document, $filter) {
        var $ctrl = this

        $scope.data = []

        $scope.peripherals = []

        $scope.events = [
            { "id": "searching", "text": "While searching" },
            { "id": "connecting", "text": "While connecting" },
            { "id": "connected", "text": "While connected" },
            { "id": "configuration", "text": "While configuration" },
            { "id": "onPgm", "text": "On program" },
            { "id": "onPvw", "text": "On preview" },
            { "id": "live", "text": "While live" },
            { "id": "rec", "text": "While recording" },
        ]   
        
        $scope.targets = [
            { "id": 1, "name": "internal" },
            { "id": 2, "name": "GoStream" }
        ]

        $scope.findTargetById = function(id) {
             if(id === -1) return ""
            return $scope.targets.find((item) => item.id === id).name 
        }

        $scope.findPifById = function(pifId) {
            var selected = [];
            if(pifId) {
                selected = $filter('filter')($scope.peripherals, {id: pifId});
            }
            return selected.length ? selected[0].name : 'Not set';
        }

        $http.get("/triggers")
            .then(function(response) {                
                $scope.data = response.data
            });
        $http.get("/peripherals")
            .then(function(response) {                
                $scope.peripherals = response.data
            });
            
        $scope.brightnessChange = function(trigger) {
            $http.put("/triggers/" + trigger.id + "/brightness", trigger).then(function(response) {})
        }

        $scope.commit = function() {
            $http.put("/commit", {}).then(function(response) {
                showSuccessMessage("Trigger updated successfully")
            })
        }

        $scope.new = function(parentSelector) {
            var parentElem = parentSelector ? 
            angular.element($document[0].querySelector('.modal-demo ' + parentSelector)) : undefined;
            var newScope = {
            "data": {
                    "id": $scope.data.length,
                    "peripheral": $scope.peripherals[0],
                    "event": "onPvw",
                    "srcId": "1",
                    "colour": "00FF00",
                    "brightness": 60
                }
            }

             var modalInstance = $uibModal.open({
            templateUrl: 'trigger-details.template.html',
            animation: false,
            controller: 'triggerDetailsCtrl',
            controllerAs: '$ctrl',
            appendTo: parentElem,
            backdrop: false, 
            resolve: {
                data: function() {
                    return newScope.data;
                },
                peripherals: function() {
                    return $scope.peripherals
                },
                events: function() {
                    return $scope.events
                }
            }
          })

          modalInstance.result.then(function(data) {
              $http.post("/triggers/" + data.id, data).then(function(response) {
                showSuccessMessage(data.id + " created successfully")
                $http.get("/triggers/" + data.id)
                .then(function(response) {                
                    $scope.data[data.id] = response.data
                });
              })
          });
        }

       $scope.delete_trigger = function(id) {
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
        
      $scope.open = function(parentSelector, id) {
        var parentElem = parentSelector ? 
        angular.element($document[0].querySelector('.modal-demo ' + parentSelector)) : undefined;
        var modalInstance = $uibModal.open({
            templateUrl: 'trigger-details.template.html',
            animation: false,
            controller: 'triggerDetailsCtrl',
            controllerAs: '$ctrl',
            appendTo: parentElem,
            backdrop: false, 
            size: 'lg',
            resolve: {
                data: function() {
                    return $scope.data[id];
                },
                peripherals: function() {
                    return $scope.peripherals
                },
                events: function() {
                    return $scope.events
                }
            }
        });

        modalInstance.result.then(function(data) {
            console.log("data", data)
            $http.put("/triggers/" + data.id, data).then(function(response) {
            showSuccessMessage(data.id + " created successfully")
            $http.get("/triggers/" + data.id)
                .then(function(response) {                
                    $scope.data[data.id] = response.data
                });
            }, function() {
          });
        })
    };
})
    
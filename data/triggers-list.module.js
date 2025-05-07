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
    function($scope, $http, $uibModal, $document) {
        var $ctrl = this

        $scope.data = [
            /*{
                "id": 0,
                "peripheral": "LED_0",
                "event": "onPvw",
                "srcId": "1",
                "colour": "00FF00",
                "brightness": 60
            }, 
            {
                "id": 1,
                "peripheral": "LED_0",
                "event": "onPgm",
                "srcId": "1",
                "colour": "FF0000",
                "brightness": 60
            }*/
        ]

        $scope.peripherals = [
            //"LED_0",
            //"LED_1",
        ]

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

        $http.get("/triggers")
            .then(function(response) {                
                $scope.data = response.data
            });
        $http.get("/peripherals")
            .then(function(response) {                
                $scope.peripherals = Object.keys(response.data)
            });
            
        $scope.brightnessChange = function(triggerId) {
            $http.post("/triggers/" + triggerId, $scope.data[triggerId])
        }


        $scope.commit = function() {
            $http.put("/commit").then(function(response) {
                showSuccessMessage("saved successfully")
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
    
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

        $scope.data = []
         /* {
                "id": 0,
                "event": "onPvw",
                "srcId": "Smart",
                "peripheral": 0,
                "colour": "#00FF00",
                "brightness": 60
            }, 
            {
                "id": 1,
                "event": "onPgm",
                "srcId": "Smart",
                "peripheral": 1,
                "colour": "#FF0000",
                "brightness": 60
            }] */

        $scope.peripherals = []
        /* {
                "id": 0,
                "name": "Operator",
                "type": "WS2811",
                "rgbOrder": "RGB",
                "pwrPin": 19,
                "ctrlPin": 18,
                "count": 1
            }, 
            {
                "id": 1,
                "name": "REC Indicator",
                "type": "WS2811",
                "rgbOrder": "RGB",
                "pwrPin": 14,
                "ctrlPin": 12,
                "count": 1
            }]*/

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
                $scope.peripherals = response.data
            });
            
        $scope.brightnessChange = function(triggerId) {
            $http.post("/triggers/" + triggerId, $scope.data[triggerId])
        }


        $scope.commit = function() {
            $http.post("/commit").then(function(response) {
                showSuccessMessage("Triggers saved successfully")
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

        modalInstance.result.then(function(id) {
              $http.get("/triggers/" + id)
                .then(function(response) {                
                    $scope.data[id] = response.data
                });
            }, function() {
        });
    };
    }
  )
    
'use strict';

angular.module('peripheralsList', [
  'peripheralDetails',
  'ui.bootstrap'
]);

angular.module('peripheralsList').
  component('peripheralsList', {
    templateUrl: 'peripherals-list.template.html',
    controller: 'peripheralsListCtrl'
   }) 
   
angular.module('peripheralsList').
  controller('peripheralsListCtrl', 
    function($scope, $http, $uibModal, $document) {
        var $ctrl = this
        $scope.data = [
            {
                "id": 0,
                "name": "LED_0",
                "type": "WS2811",
                "rgbOrder": "RGB",
                "pwrPin": 19,
                "ctrlPin": 18,
                "count": 1
            }] 

        $http.get("/peripherals")
            .then(function(response) {                
                $scope.data = response.data
            });
            
        $scope.commit = function() {
            $http.put("/commit").then(function(response) {
                showSuccessMessage("Peripherals saved successfully")
            })
        }

        $scope.new = function() {
          alert("NEW PERIPHERAL")
        }

        $scope.delete_pif = function(id) {
          alert("DELETE PERIPHERAL " + id)
        }
        
      $scope.open = function(parentSelector, id) {
        var parentElem = parentSelector ? 
        angular.element($document[0].querySelector('.modal-demo ' + parentSelector)) : undefined;
        var modalInstance = $uibModal.open({
            templateUrl: 'peripheral-details.templ.html',
            animation: false,
            controller: 'peripheralDetailsCtrl',
            controllerAs: '$ctrl',
            appendTo: parentElem,
            backdrop: false, 
            size: 'lg',
            resolve: {
                data: function() {
                    return $scope.data[id];
                },
            }
        });

        modalInstance.result.then(function(id) {
              $http.get("/peripherals/" + id)
                .then(function(response) {                
                    $scope.data[id] = response.data
                });
            }, function() {
        });
    };
    }
  )
    
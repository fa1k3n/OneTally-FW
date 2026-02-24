'use strict';

angular.module('peripheralsList', [
  'ui.bootstrap'
]);

angular.module('peripheralsList').
  component('peripheralsList', {
    templateUrl: 'peripherals-list.template.html',
    controller: 'peripheralsListCtrl'
   }) 
   
angular.module('peripheralsList').
  controller('peripheralsListCtrl', 
    function($scope, $http, $filter) {
        var $ctrl = this
        $scope.data = []

        $scope.rgbOrders = [
          { id: 1, name: "RGB" },
          { id: 2, name: "RBG" },
          { id: 3, name: "GRB" },
          { id: 4, name: "GBR" },
          { id: 5, name: "BRG" },
          { id: 6, name: "BGR" }
        ]

        $scope.boardTypes = [
          { id: 1, name: "WS2811" }
        ]

        $scope.showBoardType = function(pif) {
          var selected = [];
          if(pif.type) {
            selected = $filter('filter')($scope.boardTypes, {id: pif.type});
          }
          return selected.length ? selected[0].name : 'Not set';
        }

        $scope.showRgbOrder = function(pif) {
          var selected = [];
          if(pif.rgbOrder) {
            selected = $filter('filter')($scope.rgbOrders, {id: pif.rgbOrder});
          }
          return selected.length ? selected[0].name : 'Not set';
        }

        $http.get("/peripherals")
            .then(function(response) {                
                $scope.data = response.data
            });
            
        $scope.save_pif = function(pif) {
             $http.put("/peripherals/" + pif.id, pif).then(function(response) {
              $http.put("/commit", {}).then(function(response) {
                showSuccessMessage("Peripheral updated successfully")
              })

              $http.get("/peripherals/" + pif.id)
                .then(function(response) {                
                  let index = $scope.data.findIndex((item) => item.id === pif.id)  
                  $scope.data[index] = response.data
                });
            });
          }

        $scope.delete_pif = function(pif) {
          /*$http.delete("/peripherals/" + name).then(function(response) {
            showSuccessMessage(name + " deleted successfully")
            $http.get("/peripherals")
              .then(function(response) {                
                $scope.data = response.data
              });
          }, function(resp) {
            alert ("Failed to delete " + resp)
          })*/
        }
        
  })
    
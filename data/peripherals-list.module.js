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
            "name": "LED 0",
            "type": "WS2811",
            "rgbOrder": "RGB",
            "pwrPin": 19,
            "ctrlPin": 18,
            "count": 1
          },
          {   
            "id": 1,
            "name": "LED 1",
            "type": "WS2811",
            "rgbOrder": "RGB",
            "pwrPin": 19,
            "ctrlPin": 18,
            "count": 1
          },
        ]

        $http.get("/peripherals")
            .then(function(response) {                
                $scope.data = response.data
            });
            
        $scope.commit = function() {
            $http.put("/commit", {}).then(function(response) {
                showSuccessMessage("saved successfully")
            })
        }

        $scope.new = function(parentSelector) {
          var parentElem = parentSelector ? 
          angular.element($document[0].querySelector('.modal-demo ' + parentSelector)) : undefined;
          var newScope = {
            data: {
              "name": "",
              "type": "WS2811",
              "rgbOrder": "RGB",
              "pwrPin": 0,
              "ctrlPin": 0,
              "count": 1
            }      
          }

          var modalInstance = $uibModal.open({
            templateUrl: 'peripheral-details.templ.html',
            animation: false,
            controller: 'peripheralDetailsCtrl',
            controllerAs: '$ctrl',
            appendTo: parentElem,
            backdrop: false, 
            size: 'lg',
            resolve: {
                name: function() {
                  return newScope.name;
                },
                data: function() {
                    return newScope.data;
                },
            }
          })

          modalInstance.result.then(function(pifData) {
            $http.post("/peripherals", pifData).then(function(response) {
              showSuccessMessage(response.data.name + " updated successfully")
              $scope.data.push(response.data)

             /* $http.get("/peripherals/" + pifData.id)
                .then(function(response) {                
                  let index = $scope.data.findIndex((item) => item.id === pifData.id)  
                  $scope.data[index] = response.data

                });
            }, function() {

            $http.post("/peripherals/" + ret.name, ret.data).then(function(response) {
                showSuccessMessage(ret.name + " created successfully")
                $http.get("/peripherals/" + ret.name)
                .then(function(response) {                
                    $scope.data[ret.name] = response.data
                });
            })*/
          });
        })
      }

        $scope.delete_pif = function(name) {
          $http.delete("/peripherals/" + name).then(function(response) {
            showSuccessMessage(name + " deleted successfully")
            $http.get("/peripherals")
              .then(function(response) {                
                $scope.data = response.data
              });
          }, function(resp) {
            alert ("Failed to delete " + resp)
          })
        }
        
      $scope.open = function(parentSelector, name) {
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
                name: function() {
                  return name;
                },
                data: function() {
                    return $scope.data[name];
                },
            }
        });

        modalInstance.result.then(function(pifData) {
            $http.put("/peripherals/" + pifData.id, pifData).then(function(response) {
              showSuccessMessage(pifData.name + " updated successfully")

              $http.get("/peripherals/" + pifData.id)
                .then(function(response) {                
                  let index = $scope.data.findIndex((item) => item.id === pifData.id)  
                  $scope.data[index] = response.data

                });
            }, function() {
          });
        })
    };
  })
    
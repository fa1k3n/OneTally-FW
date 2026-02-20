'use strict';

angular.module('targetsList', [
  'targetDetails',
  'ui.bootstrap'
]);

angular.module('targetsList').
  component('targetsList', {
    templateUrl: 'targets-list.template.html',
    controller: 'targetsListCtrl'
   }) 
   
angular.module('targetsList').
  controller('targetsListCtrl', 
    function($scope, $http, $uibModal, $document, $filter) {
        var $ctrl = this
        $scope.data = [
          {  
            "id": 0,
            "name": "gostream_1",
            "type": "GoStream D8",
            "address": "192.168.255.131",
            "enabled": true
          },
        ]

        $scope.supportedTypes = [
          { id: 1, name: "GoStream D8"},
          { id: 2, name: "GoStream Duet"},
          { id: 3, name: "GoStream Solo"}
        ]

        $http.get("/targets")
            .then(function(response) {                
                $scope.data = response.data
            });
         
        $scope.checkName = function(data, id) {

        }

        $scope.checkAddress = function(data, id) {

        }

        $scope.showType = function(target) {
          var selected = [];
          if(target.type) {
            selected = $filter('filter')($scope.supportedTypes, {id: target.type});
          }
          return selected.length ? selected[0].name : 'Not set';
        }

        $scope.saveTarget = function(target) {
          $http.put("/targets/" + target.id, target).then(function(response) {
              $http.put("/commit", {}).then(function(response) {
                showSuccessMessage(target.name + " updated successfully")
              })

              $http.get("/targets/" + target.id)
                .then(function(response) {                
                  let index = $scope.data.findIndex((item) => item.id === target.id)  
                  $scope.data[index] = response.data
                });
          });
        }

        $scope.commit = function() {
                      alert("DATA", $scope.data[0]);

            $http.put("/commit", {}).then(function(response) {
                showSuccessMessage("saved successfully")
            })
        }

        $scope.new = function(parentSelector) {
          var parentElem = parentSelector ? 
          angular.element($document[0].querySelector('.modal-demo ' + parentSelector)) : undefined;
          var newScope = {
            data: {
              "id": 0,
              "name": "gostream_1",
              "type": "GoStream D8",
              "address": "",
              "enabled": true
            }      
          }

          var modalInstance = $uibModal.open({
            templateUrl: 'target-details.templ.html',
            animation: false,
            controller: 'targetDetailsCtrl',
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
            $http.post("/targets", pifData).then(function(response) {
              showSuccessMessage(response.data.name + " updated successfully")
              $scope.data.push(response.data)
          });
        })
      }

        $scope.delete_target = function(name) {
          $http.delete("/targets/" + name).then(function(response) {
            showSuccessMessage(name + " deleted successfully")
            $http.get("/targets")
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
            templateUrl: 'target-details.templ.html',
            animation: false,
            controller: 'targetDetailsCtrl',
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

        modalInstance.result.then(function(targetData) {
            $http.put("/targets/" + targetData.id, targetData).then(function(response) {
              showSuccessMessage(targetData.name + " updated successfully")

              $http.get("/target/" + targetData.id)
                .then(function(response) {                
                  let index = $scope.data.findIndex((item) => item.id === targetData.id)  
                  $scope.data[index] = response.data

                });
            }, function() {
          });
        })
    };
  })
    
package Paquette::Controller::Store;

use strict;
use warnings;
use parent 'Catalyst::Controller';
use Data::Dumper;

=head1 NAME

Paquette::Controller::Store - Catalyst Controller

=head1 DESCRIPTION

Catalyst Controller.

=head1 METHODS

=cut

sub auto : Local {
    my ( $self, $c ) = @_;

    $c->stash->{random_image} = $c->subreq(
        '/webtools/random_images', { template => 'webtools/random_image.tt2' }
    );

}

=head2 index

Index

=cut

sub index :Chained('base') :PathPart('') :Args(0) {
    my ( $self, $c ) = @_;

    $c->stash->{template} = 'store/categories.tt2';

    return;
}

sub base :Chained('/') :PathPart('store') :CaptureArgs(0) {
    my ( $self, $c ) = @_;


}

sub load : Chained('base') :PathPart('') :CaptureArgs(1) {
    my ( $self, $c, $id ) = @_;
    my $categories;

    if($id) {

        $c->stash->{id} = $id;
        $categories = [$c->model('PaquetteDB::Categories')->search({
            parent_id => $id
        })];

    } else {

        $c->response->status(404);
        $c->detach;

    }

    if ($categories) {

        $c->stash->{'categories'} = $categories;

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}

sub subcategories_base : Chained('base') :PathPart('') : CaptureArgs(1) {
    my ( $self, $c, $arg ) = @_;
    
    $c->stash->{category_url_name} = $arg;

    # Get my category id
    my $category = $c->model('PaquetteDB::Categories')->search({
        url_name => $c->stash->{category_url_name}
     })->single;

    # If our category was retreived then we set parameters
    if ($category) {
        $c->stash->{category_id}    = $category->id;
        $c->stash->{category_name}  = $category->name;
    }

    #$c->stash->{random_image} = $c->subreq(
    #    '/webtools/random_images', { template => 'webtools/random_image.tt2' }
    #);


    # Get all subcategories under this category
    $c->stash->{subcategories} = [$c->model('PaquetteDB::Categories')->search({
        parent_id => $c->stash->{category_id}
    })];

}


=head2 subcategories

Display sub categories inside this category

=cut

sub subcategory : Chained('subcategories_base') : PathPart('') : Args(0) {
    my ( $self, $c ) = @_;
    my $debug;


    if ($c->stash->{categories}) {

        $c->stash->{template} = 'store/subcategories.tt2';

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}

sub products_base : Chained('subcategories_base') :PathPart('') : CaptureArgs(1) {
    my ( $self, $c, $arg ) = @_;
    
    $c->stash->{subcategory_url_name} = $arg;

    # Get my category id
    my $subcategory = $c->model('PaquetteDB::Categories')->search({
        url_name => $c->stash->{subcategory_url_name}
     })->single;

    # If our subcategory was found then we set parameters
    if ($subcategory) {
        $c->stash->{subcategory_id}     = $subcategory->id;
        $c->stash->{subcategory_name}   = $subcategory->name;
    }

    # query database to create array of subcats
    $c->stash->{products} = [$c->model('PaquetteDB::Product')->search({
        category_id => $c->stash->{subcategory_id}
    })];


}

sub products : Chained('products_base') : PathPart('') : Args(0) {
    my ( $self, $c ) = @_;

    if ($c->stash->{products}) {

        $c->stash->{template} = 'store/products.tt2';

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}

sub items_base : Chained('products_base') :PathPart('') : CaptureArgs(1) {
    my ( $self, $c, $arg ) = @_;

    $c->stash->{item_url_name} = $arg;

    # query database to create array of subcats
    $c->stash->{items} = $c->model('PaquetteDB::Product')->search({
        url_name => $c->stash->{item_url_name}
    })->single;

}

sub item : Chained('items_base') : PathPart('') : Args(0) {
    my ( $self, $c ) = @_;

    if ($c->stash->{items}) {

        $c->stash->{template} = 'store/items.tt2';

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}


=head1 AUTHOR

Lester Ariel Mesa,,305-402-6717,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
